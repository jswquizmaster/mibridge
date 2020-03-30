/*
Copyright(c) 2020 Johannes Schulte-Wuelwer

Distributed under GPL-3.0 license
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>

#include <string>
#include <iostream>
#include <sstream>

#include "SmartLight.h"
#include "MiLight.h"
#include "HeiLight.h"
#include "external/jsoncpp/include/json/json.h"
#include <RF24/RF24.h>

#include "PL1167_nRF24.h"

extern "C" {
    #include "external/MQTT-C/include/mqtt.h"
}

#define DEFAULT_ADDRESS     "localhost"
#define DEFAULT_PORT        "1883"
#define DEFAULT_TOPIC       "milight/#"

using namespace std;

RF24 radio(RPI_V2_GPIO_P1_22, RPI_V2_GPIO_P1_24, BCM2835_SPI_SPEED_1MHZ);
PL1167_nRF24 prf(radio);
HeiLight heiLight(&prf);
MiLight miLight(&prf);

void publish_callback(void** unused, struct mqtt_response_publish *published) 
{
    // note that published->topic_name is NOT null-terminated (here we'll change it to a c-string)
    char* topicName = (char*) malloc(published->topic_name_size + 1);
    memcpy(topicName, published->topic_name, published->topic_name_size);
    topicName[published->topic_name_size] = '\0';

    string s(topicName);
    string tmp;
    int remoteID = 0;
    string bulbType;
    int groupID = 0;
    istringstream tokenStream(s);
    SmartLight *smartLight;
    
    getline(tokenStream, tmp, '/');
    getline(tokenStream, tmp, '/');
    if (tmp.rfind("0x", 0) == 0) {
        remoteID = stoi(tmp.substr(2, tmp.size() - 2), 0, 16);
    } else {
        remoteID = stoi(tmp, 0, 10);
    }
    getline(tokenStream, bulbType, '/');
    getline(tokenStream, tmp, '/');
    groupID = stoi(tmp, 0, 10);
    
    if(bulbType == "hei") {
        smartLight = &heiLight;
    } else if(bulbType == "rgbw") {
        smartLight = &miLight;
    }

    Json::Value root;
    Json::Reader reader;

    char* payloadptr = (char*) published->application_message;
    payloadptr[published->application_message_size] = '\0';
    
    bool parsingSuccessful = reader.parse( payloadptr , root );
    if (parsingSuccessful) {
        for (auto const& id : root.getMemberNames()) {
            if (id == "status") {
                string status = root[id].asString();
                if (status == "ON") {
                    smartLight->switchON(remoteID, groupID);
                } else if (status == "OFF") {
                    smartLight->switchOFF(remoteID, groupID);
                }
            } else if (id == "hue") {
                int hue = stoi(root[id].asString());
                smartLight->setColor(remoteID, groupID, hue);
            } else if (id == "brightness") {
                int brightness = stoi(root[id].asString());
                smartLight->setBrightness(remoteID, groupID, brightness);
            } else if (id == "color") {
                int colorTemp = stoi(root[id].asString());
                smartLight->setColorTemperature(remoteID, groupID, colorTemp);
            }
        }
    }

    //printf("Received publish('%s'): %s\n", topicName, payloadptr);

    free(topicName);
}


int main(int argc, char* argv[])
{
    struct addrinfo hints = {0};

    hints.ai_family = AF_UNSPEC; // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // Must be TCP
    int sockfd = -1;
    int rv;
    struct addrinfo *p, *servinfo;

    /*** get config from enviroment variables ***/
    const char* address = getenv("MIBRIDGE_ADDRESS");
    if (address == NULL ) address = DEFAULT_ADDRESS;
    const char* port = getenv("MIBRIDGE_PORT");
    if (port == NULL ) port = DEFAULT_PORT;
    const char* topic = getenv("MIBRIDGE_TOPIC");
    if (topic == NULL ) topic = DEFAULT_TOPIC;

    /*** open the non-blocking TCP socket (connecting to the broker) ***/
    // get address information
    rv = getaddrinfo(address, port, &hints, &servinfo);
    if(rv != 0) {
        fprintf(stderr, "Failed to open socket (getaddrinfo): %s\n", gai_strerror(rv));
        return -1;
    }

    // open the first possible socket
    for(p = servinfo; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1) continue;

        // connect to server
        rv = connect(sockfd, servinfo->ai_addr, servinfo->ai_addrlen);
        if(rv == -1) continue;
        break;
    }

    if(rv == -1) {
        fprintf(stderr, "Failed to open socket (connect): %s, %s, %s\n", address, port, gai_strerror(rv));
        //return -1;
    }

    // free servinfo
    freeaddrinfo(servinfo);

    // make non-blocking
    if (sockfd != -1) 
        fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL) | O_NONBLOCK);
    else {
        perror("Failed to open socket: ");
        exit(EXIT_FAILURE);
    }

    /*** setup a client ***/
    struct mqtt_client client;
    uint8_t sendbuf[2048]; // sendbuf should be large enough to hold multiple whole mqtt messages
    uint8_t recvbuf[1024]; // recvbuf should be large enough any whole mqtt message expected to be received
    mqtt_init(&client, sockfd, sendbuf, sizeof(sendbuf), recvbuf, sizeof(recvbuf), publish_callback);
    // Create an anonymous session
    const char* client_id = NULL;
    // Ensure we have a clean session
    uint8_t connect_flags = MQTT_CONNECT_CLEAN_SESSION;
    // Send connection request to the broker.
    mqtt_connect(&client, client_id, NULL, NULL, 0, NULL, NULL, connect_flags, 400);

    // check that we don't have any errors
    if (client.error != MQTT_OK) {
        fprintf(stderr, "error: %s\n", mqtt_error_str(client.error));
        exit(EXIT_FAILURE);
    }

    /*** subscribe ***/
    mqtt_subscribe(&client, topic, 0);

    printf("%s listening for '%s' messages.\n", argv[0], topic);
    
    while(1) 
    {
        mqtt_sync(&client);
        usleep(100000U);
    }

    return 0;
    
}
