#include "model/model.h"
#include <sys/socket.h>
#include <stdint.h>
#include <stdlib.h>
#include <netdb.h>
#include <errno.h>
#include <esp_log.h>


static const char *TAG = __FILE_NAME__;


int network_init(void) {
    return 0;
}

void network_connect(char *ssid, char *psk) {
    (void)ssid;
    (void)psk;
}

wifi_status_t network_status(char *ssid) {
    return WIFI_INACTIVE;
}



void network_scan(void) {}


int network_read_wifi_scan(wifi_network_t **networks) {
    (void)networks;
    return 0;
}


void network_save_config(void) {}


int network_get_ip_address(char *ifname, char *ipaddr) {
    (void)ifname;
    (void)ipaddr;
    return 0;
}


int network_ntp_update(time_t *time) {
#define NTP_TIMESTAMP_DELTA 2208988800ULL

    int   sockfd;
    int   portno    = 123;                // NTP UDP port number.
    char *host_name = "pool.ntp.org";     // NTP server host-name.

    // Structure that defines the 48 byte NTP packet protocol.
    typedef struct {
        uint8_t li_vn_mode;     // Information bitfield
        uint8_t stratum;        // Eight bits. Stratum level of the local clock.
        uint8_t poll;           // Eight bits. Maximum interval between successive messages.
        uint8_t precision;      // Eight bits. Precision of the local clock.

        uint32_t rootDelay;          //  Total round trip delay time.
        uint32_t rootDispersion;     //  Max error aloud from primary clock
        uint32_t refId;              //  Reference clock identifier.

        uint32_t refTm_s;     //  Reference time-stamp seconds.
        uint32_t refTm_f;     //  Reference time-stamp fraction of a second.

        uint32_t origTm_s;     //  Originate time-stamp seconds.
        uint32_t origTm_f;     //  Originate time-stamp fraction of a second.

        uint32_t rxTm_s;     // Received time-stamp seconds.
        uint32_t rxTm_f;     // Received time-stamp fraction of a second.

        uint32_t txTm_s;     // Transmit time-stamp seconds.
        uint32_t txTm_f;     // Transmit time-stamp fraction of a second.

    } ntp_packet;     // Total: 384 bits or 48 bytes.

    ntp_packet packet = {0};

    // Set the first byte's bits to 00,011,011 for li = 0, vn = 3, and mode = 3.
    packet.li_vn_mode = 0x1b;

    // Create a UDP socket, convert the host-name to an IP address, set the port
    // number, connect to the server, send the packet, and then read in the
    // return packet.
    struct sockaddr_in serv_addr;     // Server address data structure.
    struct hostent    *server;        // Server data structure.

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) {
        ESP_LOGE(TAG, "ERROR opening socket: %s", strerror(errno));
        return -1;
    }

    if ((server = gethostbyname(host_name)) == NULL) {
        ESP_LOGE(TAG, "ERROR, no such host: %s", strerror(errno));
        return -1;
    }

    // Zero out the server address structure.
    bzero((char *)&serv_addr, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;

    // Copy the server's IP address to the server address structure.
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);

    // Convert the port number integer to network big-endian style and save it
    // to the server address structure.
    serv_addr.sin_port = htons(portno);

    // Call up the server using its IP address and port number.
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        ESP_LOGE(TAG, "ERROR connecting: %s", strerror(errno));
        return -1;
    }

    // Send it the NTP packet it wants. If n == -1, it failed.
    if (write(sockfd, (char *)&packet, sizeof(ntp_packet)) != sizeof(ntp_packet)) {
        ESP_LOGE(TAG, "ERROR writing to socket: %s", strerror(errno));
        return -1;
    }

    // Wait and receive the packet back from the server. If n == -1, it failed.
    if (read(sockfd, (char *)&packet, sizeof(ntp_packet)) != sizeof(ntp_packet)) {
        ESP_LOGE(TAG, "ERROR reading from socket: %s", strerror(errno));
        return -1;
    }

    // These two fields contain the time-stamp seconds as the packet left the
    // NTP server. The number of seconds correspond to the seconds passed since
    // 1900. ntohl() converts the bit/byte order from the network's to host's
    // "endianness".
    packet.txTm_s = ntohl(packet.txTm_s);     // Time-stamp seconds.
    packet.txTm_f = ntohl(packet.txTm_f);     // Time-stamp fraction of a second.

    // Extract the 32 bits that represent the time-stamp seconds (since NTP
    // epoch) from when the packet left the server. Subtract 70 years worth of
    // seconds from the seconds since 1900. This leaves the seconds since the
    // UNIX epoch of 1970.
    *time = (time_t)(packet.txTm_s - NTP_TIMESTAMP_DELTA);

    // Print the time we got from the server, accounting for local timezone and
    // conversion from UTC time.
    char *t          = ctime((const time_t *)time);
    t[strlen(t) - 1] = '\0';
    ESP_LOGI(TAG, "Data e ora scaricate tramite NTP: %s", t);
    return 0;
}

static void network_time_sync_task(void *arg) {
    /*
    void (*cb)(time_t) = arg;
    time_t time;
    assert(network_ntp_update(&time) == 0);
    cb(time);
    pthread_exit(NULL);
    */
}


void launch_network_time_sync_task(void (*cb)(time_t)) {
    /*
    pthread_t id;
    pthread_create(&id, NULL, network_time_sync_task, (void *)cb);
    return id;
    */
}
