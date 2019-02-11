#include <oscpp/client.hpp>

#include <cstdio>
#include <string>
#include <cstdlib>
#include <cinttypes>
#include <regex>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

using std::memset;

static std::string to_string(sockaddr *in);

static bool split_host_port(const std::string &str, std::string &host, std::string &port);

struct force_packet {
    std::string mac;
    int32_t timestamp;
    int32_t value;
};

class osc_listener {
    int s = -1;
public:
    virtual ~osc_listener() {
        if (s >= 0) {
            close(s);
        }
    }

    int start(const std::string &local_host, const std::string &local_port) {
        addrinfo hints{};
        std::memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_flags = AI_PASSIVE;

        addrinfo *local;
        auto gai = getaddrinfo(!local_host.empty() ? local_host.c_str() : nullptr,
                               !local_port.empty() ? local_port.c_str() : nullptr, &hints, &local);
        if (gai) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(gai));
            return 1;
        }

        assert(local->ai_family == AF_INET || local->ai_family == AF_INET6);

        s = socket(local->ai_family, local->ai_socktype, local->ai_protocol);

        if (s < 0) {
            perror("socket");
            return 1;
        }

        if (bind(s, local->ai_addr, local->ai_addrlen) < 0) {
            perror("bind");
            return 1;
        }

        auto *a4 = reinterpret_cast<struct sockaddr_in *>(local->ai_addr);
        auto *a6 = reinterpret_cast<struct sockaddr_in6 *>(local->ai_addr);

        auto port = local->ai_family == AF_INET ? ntohs(a4->sin_port) : ntohs(a6->sin6_port);

        fprintf(stderr, "Listening on %s:%d\n", to_string(local->ai_addr).c_str(), port);

        return 0;
    }

    ssize_t recv(uint8_t *buf, size_t sz, struct sockaddr *addr, socklen_t *addr_len) {
        return ::recvfrom(s, buf, sz, 0, addr, addr_len);
    }
};

class osc_publisher {
    int s = -1;
    addrinfo *remote = nullptr;
    uint64_t counter = 0;

public:
    virtual ~osc_publisher() {
        if (remote) {
            freeaddrinfo(remote);
        }
    }

    int bind(const std::string &remote_host, const std::string &remote_port) {
        addrinfo hints{};
        std::memset(&hints, 0, sizeof(struct addrinfo));
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_flags = AI_PASSIVE;

        auto gai = getaddrinfo(remote_host.c_str(), remote_port.c_str(), &hints, &remote);
        if (gai) {
            fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(gai));
            return 1;
        }

        s = socket(remote->ai_family, remote->ai_socktype, remote->ai_protocol);

        if (s < 0) {
            perror("socket");
            return 1;
        }

        struct sockaddr_storage a{};
        memset((char *) &a, 0, sizeof(a));

        auto *a4 = reinterpret_cast<struct sockaddr_in *>(&a);
        auto *a6 = reinterpret_cast<struct sockaddr_in6 *>(&a);

        if (remote->ai_family == AF_INET) {
            a4->sin_family = AF_INET;
            a4->sin_addr.s_addr = htonl(INADDR_ANY);
            a4->sin_port = htons(0);
        } else if (remote->ai_family == AF_INET6) {
            a6->sin6_family = AF_INET;
            a6->sin6_addr = in6addr_any;
            a6->sin6_port = htons(0);
        } else {
            perror("Unknown address family");
            return 1;
        }

        auto *sa = reinterpret_cast<sockaddr *>(&a);
        if (::bind(s, sa, sizeof(a))) {
            perror("bind");
            return 1;
        }

        socklen_t len = sizeof(a);
        if (getsockname(s, sa, &len)) {
            perror("getsockname");
            return 1;
        }

        auto port = remote->ai_family == AF_INET ? ntohs(a4->sin_port) : ntohs(a6->sin6_port);

        printf("bind port=%d\n", port);

        return 0;
    }

    int publish(const force_packet &force_packet) {
        uint8_t buffer[1000];

        std::string prefix = "/breathing";
        printf("Sending OSC packet to %s: s='%s', i=%" PRId32 " i=%" PRId32 "\n",
               prefix.c_str(),
               force_packet.mac.c_str(),
               force_packet.timestamp,
               force_packet.value);

        OSCPP::Client::Packet packet(buffer, sizeof(buffer));
        packet
                .openBundle(counter++)
                .openMessage(prefix.c_str(), 3)
                .string(force_packet.mac.c_str())
                .int32(force_packet.timestamp)
                .int32(force_packet.value)
                .closeMessage()
                .closeBundle();
        ssize_t sz = packet.size();

        if (sendto(s, buffer, packet.size(), 0, remote->ai_addr, remote->ai_addrlen) != sz) {
            perror("sendto");
            return 1;
        }

        return 0;
    }
};

int on_line(osc_publisher &publisher, const std::string &line) {
    // fprintf(stderr, "parsing line: %s\n", line.c_str());

    int32_t timestamp = 0;
    std::string mac;
    std::array<int, 7> samples{};

    std::regex r("([0-9a-fA-F]{2}:[0-9a-fA-F]{2}:[0-9a-fA-F]{2}:[0-9a-fA-F]{2}:[0-9a-fA-F]{2}:[0-9a-fA-F]{2}),"
                 "([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),([0-9]+),.*");

    std::smatch matches;
    if (!std::regex_match(line, matches, r)) {
        return 0;
    }

    mac = matches[1].str();
    for (size_t i = 0; i < samples.size(); i++) {
        auto s = matches[i + 2].str();
        samples[i] = std::stoi(s);
    }

    for (auto sample : samples) {
        force_packet packet{
                .mac = mac,
                .timestamp = timestamp,
                .value = sample,
        };

        int ret = publisher.publish(packet);
        if (ret) {
            return ret;
        }
    }

    return 0;
};

struct {
    std::string remote;
    std::string local = "0.0.0.0";
    bool server = false;
} cli_options = {};

int server(const std::string &local_host, const std::string &local_port) {

    osc_listener listener;

    if (listener.start(local_host, local_port)) {
        return 1;
    }

    bool run = true;
    while (run) {
        uint8_t buf[1024];
        struct sockaddr_storage src_addr{};
        socklen_t sa_len = sizeof(src_addr);
        auto *sa = reinterpret_cast<sockaddr *>(&src_addr);
        auto *a4 = reinterpret_cast<struct sockaddr_in *>(&src_addr);
        auto *a6 = reinterpret_cast<struct sockaddr_in6 *>(&src_addr);

        auto sz = listener.recv(buf, sizeof(buf), sa, &sa_len);

        auto port = sa->sa_family == AF_INET ? ntohs(a4->sin_port) : ntohs(a6->sin6_port);

        fprintf(stderr, "got %zu bytes from %d\n", sz, port);
    }

    return 0;
}

int client(const std::string &remote_host, const std::string &remote_port) {
    osc_publisher publisher;
    publisher.bind(remote_host, remote_port);

    bool run = true;
    std::string str;
    while (run) {
        char rbuf[100];
        auto nread = read(STDIN_FILENO, rbuf, sizeof(rbuf));
        if (nread == 0 || nread == -1) {
            run = false;
            continue;
        } else {
            for (int i = 0; i < nread; i++) {
                auto c = rbuf[i];
                if (c == '\n' || c == '\r') {
                    if (!str.empty()) {
                        run = on_line(publisher, str) == 0;
                        str.clear();
                    }
                } else {
                    str += c;
                }
            }
        }
    }

    return 0;
}

static int usage(char *self, const char *msg = nullptr) {
    if (msg) {
        fprintf(stderr, "%s\n", msg);
    }
    fprintf(stderr, "usage: %s host:port\n", self);
    fprintf(stderr, "Options:\n");
    fprintf(stderr, " host:port The remote host and port send packages to, or local host+port to listen on\n");

    return EXIT_FAILURE;
}

int main(int argc, char *argv[]) {
    int opt;
    while ((opt = getopt(argc, argv, "sh")) != -1) {
        switch (opt) {
            case 's':
                cli_options.server = true;
                break;
            case 'h':
                return usage(argv[0]);
            case '?':
                /*if (optopt == 'l') {
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                } else */if (isprint(optopt)) {
                    fprintf(stderr, "Unknown option `-%c'.\n", optopt);
                } else {
                    fprintf(stderr, "Unknown option character `\\x%x'.\n", optopt);
                }
                return EXIT_FAILURE;
            default:
                break;
        }
    }

    if (optind == argc - 1) {
        if (cli_options.server) {
            cli_options.local = argv[optind];
        } else {
            cli_options.remote = argv[optind];
        }
        optind++;
    } else if (optind != argc) {
        return usage(argv[0], "Too may arguments");
    }

    if (!cli_options.server && cli_options.remote.empty()) {
        return usage(argv[0], "[host:port] is required");
    }

    if (cli_options.server && cli_options.local.empty()) {
        return usage(argv[0], "[host:port] is required");
    }

    std::string remote_host;
    std::string remote_port;

    if (!split_host_port(cli_options.remote, remote_host, remote_port)) {
        usage(argv[0], "Bad remote");
        return EXIT_FAILURE;
    }

    std::string local_host;
    std::string local_port;

    if (!split_host_port(cli_options.local, local_host, local_port)) {
        usage(argv[0], "Bad local");
        return EXIT_FAILURE;
    }

    printf("local: %s, host=%s, port=%s\n", cli_options.local.c_str(), local_host.c_str(), local_port.c_str());

    auto ret = cli_options.server ? server(local_host, local_port) : client(remote_host, remote_port);
    return ret ? EXIT_FAILURE : EXIT_SUCCESS;
}

static std::string to_string(sockaddr *in) {
    char addr[INET_ADDRSTRLEN > INET6_ADDRSTRLEN ? INET_ADDRSTRLEN : INET6_ADDRSTRLEN];

    if (in->sa_family == AF_INET) {
        auto *a4 = reinterpret_cast<sockaddr_in *>((void *) in);
        if (inet_ntop(AF_INET, &a4->sin_addr, addr, INET_ADDRSTRLEN) == nullptr) {
            return "";
        }
    } else if (in->sa_family == AF_INET6) {
        auto *a6 = reinterpret_cast<sockaddr_in6 *>((void *) in);
        if (inet_ntop(AF_INET6, &a6->sin6_addr, addr, INET6_ADDRSTRLEN) == nullptr) {
            return "";
        }
    }

    return addr;
}

static bool split_host_port(const std::string &str, std::string &host, std::string &port) {
    auto idx = str.find_last_of(':');
    if (idx == std::string::npos) {
        host = str;
    } else {
        if (str.length() > idx) {
            host = str.substr(0, idx);
            port = str.substr(idx + 1);
        }

        if (port.empty()) {
            return false;
        }
    }

    return true;
}
