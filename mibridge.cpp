/*
Copyright(c) 2020 Johannes Schulte-Wuelwer

Distributed under GPL-3.0 license
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <string>
#include <iostream>
#include <sstream>

#include "SmartLight.h"
#include "MiLight.h"
#include "HeiLight.h"
#include "external/jsoncpp/include/json/json.h"
#include <RF24/RF24.h>

#include "PL1167_nRF24.h"
#include "posix_socket.h"

extern "C" {
    #include "external/MQTT-C/include/mqtt.h"
}

#define DEFAULT_ADDRESS     "localhost"
#define DEFAULT_PORT        "1883"
#define DEFAULT_TOPIC       "milight/#"

using namespace std;

struct reconnect_state_t {
    const char* hostname;
    const char* port;
    const char* topic;
    uint8_t* sendbuf;
    size_t sendbufsz;
    uint8_t* recvbuf;
    size_t recvbufsz;
};

//RF24 radio(RPI_V2_GPIO_P1_22, RPI_V2_GPIO_P1_24, BCM2835_SPI_SPEED_1MHZ);  /* Raspberry PI 3B */
RF24 radio(402, 10);  /* Up2 x86 Device */
PL1167_nRF24 prf(radio);
HeiLight heiLight(&prf);
MiLight miLight(&prf);

void reconnect_client(struct mqtt_client* client, void **reconnect_state_vptr)
{
    struct reconnect_state_t *reconnect_state = *((struct reconnect_state_t**) reconnect_state_vptr);

    // Close the clients socket if this isn't the initial reconnect call
    if (client->error != MQTT_ERROR_INITIAL_RECONNECT) {
        close(client->socketfd);
    }

    // Perform error handling here.
    if (client->error != MQTT_ERROR_INITIAL_RECONNECT) {
        printf("reconnect_client: called while client was in error state \"%s\"\n",
               mqtt_error_str(client->error)
        );
    }

    // Open a new socket.
    int sockfd = open_nb_socket(reconnect_state->hostname, reconnect_state->port);
    if (sockfd == -1) {
        perror("Failed to open socket: ");
        exit(EXIT_FAILURE);
    }

    // Reinitialize the client.
    mqtt_reinit(client, sockfd,
                reconnect_state->sendbuf, reconnect_state->sendbufsz,
                reconnect_state->recvbuf, reconnect_state->recvbufsz
    );

    // Create an anonymous session
    const char* client_id = NULL;
    // Ensure we have a clean session
    uint8_t connect_flags = MQTT_CONNECT_CLEAN_SESSION;
    // Send connection request to the broker.
    mqtt_connect(client, client_id, NULL, NULL, 0, NULL, NULL, connect_flags, 400);

    // Subscribe to the topic.
    mqtt_subscribe(client, reconnect_state->topic, 0);
}

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

    /*** get config from enviroment variables ***/
    const char* address = getenv("MIBRIDGE_ADDRESS");
    if (address == NULL ) address = DEFAULT_ADDRESS;
    const char* port = getenv("MIBRIDGE_PORT");
    if (port == NULL ) port = DEFAULT_PORT;
    const char* topic = getenv("MIBRIDGE_TOPIC");
    if (topic == NULL ) topic = DEFAULT_TOPIC;


    /*** build the reconnect_state structure which will be passed to reconnect ***/
    struct reconnect_state_t reconnect_state;
    reconnect_state.hostname = address;
    reconnect_state.port = port;
    reconnect_state.topic = topic;
    uint8_t sendbuf[2048];
    uint8_t recvbuf[1024];
    reconnect_state.sendbuf = sendbuf;
    reconnect_state.sendbufsz = sizeof(sendbuf);
    reconnect_state.recvbuf = recvbuf;
    reconnect_state.recvbufsz = sizeof(recvbuf);

    /*** setup a client ***/
    struct mqtt_client client;

    mqtt_init_reconnect(&client,
                        reconnect_client, &reconnect_state,
                        publish_callback
    );


    printf("%s listening for '%s' messages.\n", argv[0], topic);

    while(1)
    {
        mqtt_sync(&client);
        usleep(100000U);
    }

    return 0;
}
