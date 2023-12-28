#include "../include/color.hpp"
#include <fstream>
#include <ifaddrs.h> // struct ifaddrs
#include <iomanip>
#include <iostream>
#include <map>
#include <netdb.h> // define NI_MAXHOST 1025
#include <string>
#include <sys/utsname.h> // struct utsname, uname
#include <unistd.h>      // getlogin_r, gethostname

#ifdef __FreeBSD__ // on Linux, already include in netdb.h
#include <netinet/in.h>
#include <sys/socket.h>
#endif

std::string getUsername();
std::string getHostname();
std::string getShell();
std::string getDistro();
std::string getCPUInfo();
std::string getMemoryInfo();
std::map<std::string, std::string> getIP();

void print_name(std::string name, char end = '\t') {
  std::cout << style::apply(style::FONT_STYLE::bold, style::FONT_COLOR::cyan)
            << name << style::reset() << end;
}

void print_error(std::string error) {
  std::cerr << style::apply(style::FONT_STYLE::bold, style::FONT_COLOR::red)
            << error << style::reset();
}

int main() {
  std::cout << style::apply(style::FONT_STYLE::bold, style::FONT_COLOR::white)
            << getUsername() << style::reset();
  std::cout << "@";
  std::cout << style::apply(style::FONT_STYLE::bold, style::FONT_COLOR::white)
            << getHostname() << std::endl
            << style::reset();
  std::cout << std::string(16, '=') << std::endl;

  // Shell
  print_name("Shell");
  std::cout << getShell() << std::endl;

  // OS
  print_name("Distro");
  std::cout << getDistro() << std::endl;
  struct utsname info;
  uname(&info);
  print_name("Kernel");
  std::cout << info.sysname << " " << info.release << std::endl;
  //   print_name("Distro");
  //   std::cout << info.version << std::endl;
  print_name("Arch");
  std::cout << info.machine << std::endl;

  // CPU
  print_name("CPU");
  std::cout << getCPUInfo() << std::endl;

  // Memory
  print_name("Memory");
  std::cout << getMemoryInfo() << std::endl;

  // IP
  print_name("IP", '\0');
  std::map<std::string, std::string> ips = getIP();
  for (const auto &ip : ips) {
    std::cout << '\t'
              << style::apply(style::FONT_STYLE::bold,
                              style::FONT_COLOR::purple)
              << ip.first << style::reset() << ' ' << ip.second << std::endl;
  }

  return 0;
}

std::string getUsername() {
  char username[256];
  if (getlogin_r(username, sizeof(username)) != 0) {
    perror("getlogin_r");
    exit(EXIT_FAILURE);
  }
  return std::string(username);
}

std::string getHostname() {
  char hostname[256];
  if (gethostname(hostname, sizeof(hostname)) != 0) {
    perror("gethostname");
    exit(EXIT_FAILURE);
  }
  return std::string(hostname);
}

std::string getShell() {
  const char *shell = getenv("SHELL");
  if (shell == NULL) {
    perror("getenv");
    exit(EXIT_FAILURE);
  }
  return std::string(shell);
}

std::string getDistro() {

  std::ifstream file("/etc/os-release");
  if (!file.is_open()) {
    print_error("Error opening /etc/os-release");
    return "";
  }

  std::string line;
  std::string value;
  while (std::getline(file, line)) {
    if (line.find("PRETTY_NAME") != std::string::npos) {
      size_t equal_sign_pos = line.find('=');
      if (equal_sign_pos != std::string::npos) {
        value = line.substr(equal_sign_pos + 1);
        break;
      }
    }
  }
  file.close();

  // 去掉雙引號
  if (!value.empty()) {
    value.erase(0, 1);
    value.pop_back();
  }

  return value;
}

std::string getCPUInfo() {
  std::string file_path;
#ifdef __FreeBSD__
  file_path = "/compat/linux/proc/cpuinfo";
#endif
#ifdef __linux__
  file_path = "/proc/cpuinfo";
#endif

  std::ifstream cpuinfo_file(file_path);
  if (!cpuinfo_file.is_open()) {
    print_error("Error opening " + file_path);

    return "";
  }
  std::string line;
  std::string model_name;
  while (std::getline(cpuinfo_file, line)) {
    if (line.find("model name") != std::string::npos) {
      size_t colon_pos = line.find(':');
      if (colon_pos != std::string::npos) {
        model_name = line.substr(colon_pos + 2);
        break;
      }
    }
  }
  cpuinfo_file.close();
  if (!model_name.empty()) {
    size_t newline_pos = model_name.find('\n');
    if (newline_pos != std::string::npos) {
      model_name.erase(newline_pos);
    }
  }

  return model_name;
}

std::string getMemoryInfo() {
  std::string file_path;
#ifdef __FreeBSD__
  file_path = "/compat/linux/proc/meminfo";
#endif
#ifdef __linux__
  file_path = "/proc/meminfo";
#endif

  std::ifstream meminfo_file(file_path);
  if (!meminfo_file.is_open()) {
    print_error("Error opening " + file_path);
    return "";
  }

  std::string line;
  int memTotalKB = -1;
  while (std::getline(meminfo_file, line)) {
    if (line.find("MemTotal") != std::string::npos) {
      size_t colonPos = line.find(':');
      if (colonPos != std::string::npos) {
        memTotalKB = std::stoi(line.substr(colonPos + 1));
        break;
      }
    }
  }
  meminfo_file.close();
  if (memTotalKB == -1) {
    return "";
  }
  int memTotalMiB = static_cast<int>(memTotalKB) / 1024;
  return std::to_string(memTotalMiB) + " MiB";
}

std::map<std::string, std::string> getIP() {
  std::map<std::string, std::string> ips;

  struct ifaddrs *ifaddr, *ifa;
  char host[NI_MAXHOST];
  if (getifaddrs(&ifaddr) == -1) {
    perror("getifaddrs");
    exit(EXIT_FAILURE);
  }
  for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr == NULL)
      continue;

    int family = ifa->ifa_addr->sa_family;

    if (family == AF_INET) {
      int s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host,
                          NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
      if (s != 0) {
        printf("getnameinfo() failed: %s\n", gai_strerror(s));
        exit(EXIT_FAILURE);
      }
      ips[std::string(ifa->ifa_name)] = std::string(host);
    }
  }

  freeifaddrs(ifaddr);
  return ips;
}