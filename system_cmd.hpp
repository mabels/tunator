#ifndef __SystemCmd__
#define __SystemCmd__

#include <vector>
#include <string>
#include <sstream>

#define ELPP_THREAD_SAFE
#include <easylogging++.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

// #include <boost/process/context.hpp>
#include <boost/optional.hpp>
#include <boost/asio.hpp>
#include <boost/asio/buffer.hpp>
#include <boost/asio/placeholders.hpp>
//#include <boost/process.hpp>
#include <boost/array.hpp>
#include <boost/bind.hpp>

#include <unistd.h>
#include <fcntl.h>

#include <result.hpp>

class FileDescriptor {
  int fd;
public:
  int getFd() const { return fd; }
  FileDescriptor(int fd) : fd(fd) {}
  ~FileDescriptor() { close(fd); }
  void nonBlocking() const {
    int saved_flags = fcntl(this->fd, F_GETFL);
    fcntl(this->fd, F_SETFL, saved_flags & ~O_NONBLOCK);
  }
};

class Pipe {
  std::shared_ptr<FileDescriptor> read;
  std::shared_ptr<FileDescriptor> write;
public:
  int getRead() const { return read->getFd(); }
  int getWrite() const { return write->getFd(); }
  static Result<Pipe> create() {
    int pipeFds[2];
    if (pipe(pipeFds)) {
        return Err<Pipe>("Can't create the pipe");
    }
    Pipe ret;
    ret.read = std::shared_ptr<FileDescriptor>(new FileDescriptor(pipeFds[0]));
    ret.write = std::shared_ptr<FileDescriptor>(new FileDescriptor(pipeFds[1]));
    return Ok(ret);
  }
  void nonBlocking() const {
    read->nonBlocking();
    //read->nonBlocking();
  }
};

class SystemResult {
  public:
  std::stringstream sout;
  std::stringstream serr;
  std::string cmd;
  bool ok;
  int statusCode;
  int exitCode;
  int waitPid;
  static SystemResult err() {
    SystemResult ret;
    ret.ok = false;
    return ret;
  }
};

class SystemCmd {
private:
  std::vector<std::string> args;
  std::vector<std::string> envs;
  const std::string exec;
  boost::optional<int> status;
public:
  SystemCmd(const std::string &cmd) : exec(cmd) {
      arg(exec);
  }
  SystemCmd(const char *cmd) : exec(cmd) {
      arg(exec);
  }

  boost::optional<int> getStatus() {
    return status;
  }

  SystemCmd &env(const char *key, const char *value)  {
    std::string kv(key);
    kv += "=";
    kv += value;
    envs.push_back(kv);
    return *this;
  }

  template<typename Y>
  SystemCmd &arg(Y part) { args.push_back(std::to_string(part)); return *this; }
  SystemCmd &arg(const char *part) { args.push_back(part); return *this; }
  SystemCmd &arg(const std::string &part) { args.push_back(part); return *this;}

  std::string dump() const {
    std::stringstream ret;
    const char *space = "";
    for (auto &v : envs) {
        ret << space << v;
        space = " ";
    }
    ret << space << exec;
    space = " ";
    bool first = true;
    for (auto &v : args) {
      if (!first) {
        ret << space << v;
        space = " ";
      }
      first = false;
    }
    if (status) {
      ret << "=>" << *status;
    }
    return ret.str();
  }

  static void register_read(boost::asio::posix::stream_descriptor &ds,
                            boost::asio::mutable_buffers_1 &buf,
                            std::stringstream &output) {

        boost::asio::async_read(ds, buf, [&ds, &buf, &output]
                      (boost::system::error_code ec, std::size_t bytes_transferred) {
                      // LOG(INFO) << "read:" << ec << ":" << bytes_transferred << std::endl;
                      if (ec == boost::asio::error::eof || !ec) {
                        std::string s(boost::asio::buffer_cast<char*>(buf), bytes_transferred);
                        //std::cout << "Hello:" << bytes_transferred << ":" << s << std::endl;
                        output << s;
                        if (!ec) {
                          register_read(ds, buf, output);
                        }
                      } else if (ec) {
                        LOG(ERROR) << "register_read failed:" << ec;
                        return;
                      }
                    });
  }

  pid_t launch(int stdOut, int stdErr) const {
      //signal(SIGCHLD, SIG_IGN);
      pid_t pid = fork();
      if (pid == 0) {
        close(1); dup2(stdOut, 1);
        close(2); dup2(stdErr, 2);
        char *argv[args.size()+1];
        argv[args.size()] = 0;
        for (size_t i = 0; i < args.size(); ++i) {
            argv[i] = (char *)args[i].c_str();
        }
        char *envp[envs.size()+1];
        envp[envs.size()] = 0;
        for (size_t i = 0; i < envs.size(); ++i) {
            envp[i] = (char *)envs[i].c_str();
        }
        if (execve(exec.c_str(), argv, envp) == -1) {
          // ACHTUNG HACK
          std::cout << "[exec failed]" << std::flush;
          std::cerr << "[exec failed]" << std::flush;
          exit(42);
        }
      } else {
        close(stdOut);
        close(stdErr);
      }
      return pid;
  }

  static void handleSigChild(boost::asio::signal_set &sigchld, SystemResult &sr) {
    sigchld.async_wait([&sigchld, &sr](const boost::system::error_code&, int) -> void {
      if (sr.waitPid > 0) {
        while (waitpid(sr.waitPid, &sr.statusCode, WNOHANG) > 0) {}
        if (WIFEXITED(sr.statusCode)) {
          // LOG(INFO) << "EXIT sigchld:" << sr.waitPid << ":" << sr.statusCode
          //   << ":" << WEXITSTATUS(sr.statusCode);
          sr.exitCode = WEXITSTATUS(sr.statusCode);
        } else {
          LOG(INFO) << "Restart sigchld:" << sr.waitPid << ":" << sr.statusCode;
          handleSigChild(sigchld, sr);
        }
      }
    });
  }

  SystemResult run() {
    SystemResult sr;
    auto stdoutPipe = Pipe::create();
    auto stderrPipe = Pipe::create();
    if (stdoutPipe.isErr() || stderrPipe.isErr()) {
          sr.ok = false;
          return sr;
    }
    boost::asio::io_service io_service;
    boost::asio::signal_set sigchld(io_service, SIGCHLD);
    SystemCmd::handleSigChild(sigchld, sr);
    sr.waitPid = launch(stdoutPipe->getWrite(), stderrPipe->getWrite());
    stdoutPipe->nonBlocking();
    stderrPipe->nonBlocking();
    boost::asio::posix::stream_descriptor sdOut(io_service, stdoutPipe->getRead());
    boost::asio::posix::stream_descriptor sdErr(io_service, stderrPipe->getRead());
    boost::array<char, 4096> soutArray;
    boost::array<char, 4096> serrArray;
    auto soutBuffer = boost::asio::buffer(soutArray);
    auto serrBuffer = boost::asio::buffer(serrArray);
    register_read(sdOut, soutBuffer, sr.sout);
    register_read(sdErr, serrBuffer, sr.serr);
    // char buf[1000];
    // int len;
    // LOG(INFO) << "stdout:" << (len=read(stdoutPipe->getRead(), buf, sizeof(buf))) << std::endl;
    // LOG(INFO) << std::string(buf, len) << std::endl;
    // LOG(INFO) << "stdout:" << (len=read(stdoutPipe->getRead(), buf, sizeof(buf))) << std::endl;
    // LOG(INFO) << std::string(buf, len) << std::endl;
    // LOG(INFO) << "stderr:" << read(stderrPipe->getRead(), buf, sizeof(buf)) << std::endl;
    io_service.run();
    // sr.waitPid = waitpid(pid, &sr.statusCode, WEXITED);
    // sr.exitCode = WEXITSTATUS(sr.statusCode);
    //LOG(INFO) << " WIFEXITED(status):" << WIFEXITED(status) << std::endl;
    //std::cout << sr.waitPid << ":" << sr.exitCode << "--" << sr.sout.str() << "--" << sr.serr.str() << std::endl;
    sr.ok = !(sr.exitCode == 42 && sr.sout.str() == sr.serr.str() && sr.sout.str() == "[exec failed]");
    sr.cmd = this->dump();
    //LOG(INFO) << sr.ok << ":" << dump();
    return sr;
  }
};

#endif
