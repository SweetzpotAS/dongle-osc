#include <oscpp/client.hpp>

#include <array>
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
#include <termios.h>

using std::memset;

struct force_packet {
    std::string mac;
    int32_t timestamp;
    int32_t value;
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
        socklen_t socklen;

        if (remote->ai_family == AF_INET) {
            a4->sin_family = AF_INET;
            a4->sin_addr.s_addr = htonl(INADDR_ANY);
            a4->sin_port = htons(0);
            socklen = sizeof(sockaddr_in);
        } else if (remote->ai_family == AF_INET6) {
            a6->sin6_family = AF_INET;
            a6->sin6_addr = in6addr_any;
            a6->sin6_port = htons(0);
            socklen = sizeof(sockaddr_in6);
        } else {
            return 1;
        }

        if (::bind(s, reinterpret_cast<sockaddr *>(&a), socklen)) {
            perror("bind");
            return 1;
        }

	        socklen_t len = sizeof(a);
        if (getsockname(s, reinterpret_cast<sockaddr *>(&a), &len)) {
            perror("getsockname");
            return 1;
        }

//        auto port = remote->ai_family == AF_INET ? ntohs(a4->sin_port) : ntohs(a6->sin6_port);
//
//        printf("Bound to local port: %d\n", port);

        return 0;
    }

    int publish(const force_packet &force_packet) {
        uint8_t buffer[1000];

        std::string prefix = "/breathing";
        printf("Sending OSC packet to %s: s='%s' i=%" PRId32 " i=%" PRId32 "\n",
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
    std::string remote, remote_host, remote_port;
    std::string input;

    bool set_remote(const std::string &remote) {
        this->remote = remote;
        return split_host_port(remote, remote_host, remote_port);
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
} cli_options = {};

int client(const std::string &input, const std::string &remote_host, const std::string &remote_port) {


    int ifd = STDIN_FILENO;
    if (!input.empty()) {
        if ((ifd = ::open(input.c_str(), O_RDWR | O_NOCTTY)) == -1) {
            perror("Could not open input");
            return 1;
        }

        termios options = {};
        if (tcgetattr(ifd, &options)) {
            perror("tcgetattr");
            return 1;
        }

//        printf("options.c_cflag=%08x\n", options.c_cflag);
//        printf("options.c_lflag=%08x\n", options.c_lflag);
//        printf("options.c_iflag=%08x\n", options.c_iflag);
//        printf("options.c_oflag=%08x\n", options.c_oflag);
//        for (int i = 0; i < NCCS; i++) {
//            cc_t cc = options.c_cc[i];
//            printf("options.c_cc[%d]=%08x/%d\n", i, cc, cc);
//        }

        options.c_cflag &= ~PARENB;     // No parity
        options.c_cflag &= ~CSTOPB;     // 1 stop bit
        options.c_cflag &= ~CSIZE;      // 8-bit words
        options.c_cflag |= CS8;
        options.c_cflag &= ~CRTSCTS;    // No hardware-flow control
        // We run in non-canonical mode
        options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
        options.c_oflag &= ~OPOST;
        options.c_iflag &= ~BRKINT;
        options.c_cc[VMIN] = 1;
        options.c_cc[VTIME] = 5;
        cfsetispeed(&options, B19200);
        cfsetospeed(&options, B19200);

        if (tcsetattr(ifd, TCSANOW, &options)) {
            perror("tcsetattr");
            return 1;
        }
    }

    osc_publisher publisher;
    if (publisher.bind(remote_host, remote_port)) {
        return 1;
    }

    bool run = true;
    std::string str;
    while (run) {
        char rbuf[100];
        auto nread = read(ifd, rbuf, sizeof(rbuf));
        if (nread == 0 || nread == -1) {
            if (nread == -1) {
                perror("read");
            }
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
    fprintf(stderr, " -i [SERIAL PORT]\n");
    fprintf(stderr, " host:port The remote host and port send packages to\n");

    return EXIT_FAILURE;
}

int main(int argc, char *argv[]) {
    int opt;
    while ((opt = getopt(argc, argv, "i:h")) != -1) {
        switch (opt) {
            case 'i':
                cli_options.input = optarg;
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
        if (!cli_options.set_remote(argv[optind])) {
            usage(argv[0], "Bad remote");
            return EXIT_FAILURE;
        }

        optind++;
    } else if (optind != argc) {
        return usage(argv[0], "Too may arguments");
    }

    if (cli_options.remote.empty()) {
        return usage(argv[0], "[host:port] is required");
    }

    auto ret = client(cli_options.input, cli_options.remote_host, cli_options.remote_port);
    return ret ? EXIT_FAILURE : EXIT_SUCCESS;
}
