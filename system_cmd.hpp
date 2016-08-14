#include <vector>
#include <string>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

class SystemCmd {
private:
  std::vector<std::string> args;
  std::vector<std::string> envs;
  const std::string exec;
  int status;
public:
  SystemCmd(const std::string &cmd) : exec(cmd), status(-1) {
      arg(exec);
  }
  SystemCmd(const char *cmd) : exec(cmd), status(-1) {
      arg(exec);
  }

  int getStatus() {
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
    if (status >= 0) {
      ret << "=>" << status;
    }
    return ret.str();
  }
  int run() {
    pid_t pid = fork();
    if (pid == 0) {
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
      execve(exec.c_str(), argv, envp);
    }
    waitpid(pid, &status, WEXITED);
    LOG(INFO) << dump();
    return status;
  }
};
