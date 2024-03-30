/* mbed Microcontroller Library
 * Copyright (c) 2019 ARM Limited
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ipify_org_ca_root_certificate.h"
#include "openwater_cert.h"
#include "mbed.h"
#include "wifi.h"
#include "DFRobot_RGBLCD.h"
DFRobot_RGBLCD lcd(16, 2, PB_9, PB_8);
#define JSON_NOEXCEPTION
#include "json.hpp"
#include "mbed.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "platform/mbed_thread.h"
#include <string.h>
#include "HTS221Sensor.h"
#include "DevI2C.h"
#include <fstream>
#include <iostream>
#include <string.h>
#include <stdio.h>
#include <string.h>
#include <vector>
#define HASH 101
bool alarm = false;
DevI2C i2c(PB_11, PB_10);
HTS221Sensor hts221(&i2c);
#define output NULL;
// Here is the button interrupts
DigitalIn increasehour(PC_4); //red
DigitalIn increasemin(PC_1); // yellow
DigitalIn snooze(PC_0); //BLUE
PwmOut buzzer(PA_5);

InterruptIn bigbluebutton(PC_13, PullNone);
InterruptIn alarmbutton(PC_2, PullNone);


// As defined in wikipedia https://en.wikipedia.org/wiki/Rabin%E2%80%93Karp_algorithm#The_algorithm
int hash(const char *str, size_t len) {
    int res = 0;
    for (size_t i = 0; i < len; i++) {
        res = (256 * res + str[i]) % HASH;
    }
    return res;
}


struct Match {
    const char *ptr;
    size_t len;

    friend std::ostream &operator<<(std::ostream &os, const Match &m) {
        for (size_t i = 0; i < m.len; i++) {
            os << m.ptr[i];
        }
        return os;
    }

    friend std::string &operator+= (std::string &s, const Match &m) {
        for (size_t i = 0; i < m.len; i++) {
            s += m.ptr[i];
        }
        return s;
    }
};
// Find the first match of pattern in data
size_t find_first_match(const char *data, const char pattern[], int pattern_hash, size_t start_pos, size_t data_len, size_t pattern_len) {
    int current_hash = ::hash(data + start_pos, pattern_len);

    int max_base = 1;
    for (size_t i = 0; i < pattern_len - 1; i++) {
        max_base = (max_base * 256) % HASH;
    }

    for (size_t i = start_pos; i < data_len - pattern_len + 1; i++) {
        // If the hash is the same, check the content
        if (current_hash == pattern_hash && strncmp(data + i, pattern, pattern_len) == 0) {
            return i;
        }

        // Rolling hash
        current_hash = (256 * (current_hash - data[i] * max_base) + data[i + pattern_len]) % HASH;

        // Make sure hash is positive
        if (current_hash < 0) {
            current_hash += HASH;
        }
    }

    return SIZE_MAX;
}

// Find all the matches in the file using an adapted Rabin-Karp algorithm
std::vector<Match> parse_data(const char data[], const char pattern_start[], const char pattern_end[], size_t data_len) {
    std::vector<Match> matches;

    size_t pattern_start_len = strlen(pattern_start);
    size_t pattern_end_len = strlen(pattern_end);

    // Hash the patterns
    int p_start_hash =::hash(pattern_start, pattern_start_len);
    int p_end_hash = ::hash(pattern_end, pattern_end_len);

    for (size_t i = 0; i < data_len; i++) {
        // Find starting tag
        size_t pos = find_first_match(data, pattern_start, p_start_hash, i, data_len, pattern_start_len);
        if (pos == SIZE_MAX) {
            break;
        }
        pos += pattern_start_len; // move to the end of the first tag

        // Find ending tag
        size_t end = find_first_match(data, pattern_end, p_end_hash, pos, data_len, pattern_end_len);

        if (end == SIZE_MAX) {
            break;
        }

        Match m;
        m.ptr = data + pos;
        m.len = end - pos;
        matches.push_back(m);
        i = end + pattern_end_len - 1;
    }

    return matches;
}



int screen_selection = 1;
char num[30];


void humtemp_screen()
{
    lcd.init();
    lcd.display();

DevI2C i2c(PB_11, PB_10);
HTS221Sensor sensor(&i2c);
    sensor.init(nullptr);
    sensor.enable();

    while(screen_selection == 3)
    {
         lcd.clear();
        float temperature; 
        sensor.get_temperature(&temperature);
        if (sensor.get_temperature(&temperature) != 0)
        {
            printf("An error occured reading the temperature.");
        }
        printf("\nTempere: %0.2lf\n", temperature);
        lcd.setCursor(0,0);
        lcd.printf("Temp: %0.2lf C", temperature);

        float humidity;
        sensor.get_humidity(&humidity);
        if (sensor.get_humidity(&humidity) != 0)
        {
            printf("An error occured reading the humidity.");
        }
        printf("\nHumidity: %f\n", humidity);
        lcd.setCursor(0,1);
        lcd.printf("Humidity: %0.2lf", humidity);

        ThisThread::sleep_for(1000);
}

}

void weather_screen()
{

    while(screen_selection == 4)

{
    
  nsapi_size_or_error_t result;


    TLSSocket socket;

    socket.set_timeout(500);
 NetworkInterface *network = NetworkInterface::get_default_instance();
    result = socket.open(network);
      SocketAddress address;

    if (result != NSAPI_ERROR_OK) {
      printf("Failed to open TLSSocket: %s\n", get_nsapi_error_string(result));
    }

    const char host[] = "api.openweathermap.org"; // Host api.ipify.org will not work
    // Get IP address of host (web server) by name
    result = network->gethostbyname(host, &address);

    if (result != NSAPI_ERROR_OK) {
      printf("Failed to get IP address of host %s: %s\n", host,
             get_nsapi_error_string(result));
    }

    printf("IP address of server %s is %s\n", host, address.get_ip_address());

    // Set server TCP port number, 443 for HTTPS
    address.set_port(443);

    // Set the root certificate of the web site.
    // See include/ipify_org_ca_root_certificate.h for how to download the cert.
    result = socket.set_root_ca_cert(openwater_cert);

    if (result != NSAPI_ERROR_OK) {
      printf("Failed to set root certificate of the web site: %s\n",
             get_nsapi_error_string(result));
    }

    // Connect to server at the given address
    result = socket.connect(address);

    // Check result
    if (result != NSAPI_ERROR_OK) {
      printf("Failed to connect to server at %s: %s\n", host,
             get_nsapi_error_string(result));
    }

    printf("Successfully connected to server %s\n", host);

    // Create HTTP request
    const char request[] = "GET /data/2.5/weather?lat=58.3405&lon=8.59343&appid=4c838f0588132ff546a0535009699677&units=metric HTTP/1.1\r\n"
                           "Host: api.openweathermap.org\r\n"
                           "Connection: close\r\n"
                           "\r\n";

    // Send request
    result = send_request(&socket, request);

    // Check result
    if (result < 0) {
      printf("Failed to send request: %d\n", result);
    }

    // We need to read the response into memory. The destination is called a
    // buffer. If you make this buffer static it will be placed in BSS and won't
    // use stack memory.
    static char buffer[2000];

    // Read response
    result = read_response(&socket, buffer, sizeof(buffer));

    // Check result
    if (result < 0) {
      printf("Failed to read response: %d\n", result);
    }


    // Find the start and end of the JSON data.
    // If the JSON response is an array you need to replace this with [ and ]
    char *json_begin = strchr(buffer, '{');
    char *json_end = strrchr(buffer, '}');

    // Check if we actually got JSON in the response
    if (json_begin == nullptr || json_end == nullptr) {
      printf("Failed to find JSON in response\n");
    } 
    // End the string after the end of the JSON data in case the response
    // contains trailing data
    json_end[1] = 0;

    printf("JSON response:\n");
    printf("%s\n", json_begin);

    // Parse response as JSON, starting from the first {
    nlohmann::json document = nlohmann::json::parse(json_begin);

    if (document.is_discarded()) {
      printf("The input is invalid JSON\n");
    }

    // Get IP address from JSON object
    std::string description;
    float temp;
    document["main"]["temp"].get_to(temp);
    document["weather"][0]["description"].get_to(description);
    lcd.setCursor(0,0);
    lcd.printf("%s\n", description.c_str());
    lcd.setCursor(0,1);
    lcd.printf("%0.0lf C", temp);
    thread_sleep_for(500);
  }
}


void Selection_increase(void)
{
 screen_selection++;
}

void alarmoffon(void){
    alarm = !alarm;
}

using json = nlohmann::json;

int main() {
    int hournr2;
    int minnr2;
    int hour=0;
    int min=0;
    bigbluebutton.mode(PullUp);   
    bigbluebutton.fall(&Selection_increase) ;
    alarmbutton.fall(&alarmoffon);
    lcd.init();
    lcd.display();



 // Conneection to WIFI (opening socket)
 
  NetworkInterface *network = NetworkInterface::get_default_instance();
  if (!network) {
    printf("Failed to get default network interface\n");
    while (1);
  }

  nsapi_size_or_error_t result;

  do {
    printf("Connecting to the network...\n");
    result = network->connect();

    if (result != NSAPI_ERROR_OK) {
      printf("Failed to connect to network: %d\n", result);
    }
  } while (result != NSAPI_ERROR_OK);

  SocketAddress address;
  result = network->get_ip_address(&address);

  if (result != NSAPI_ERROR_OK) {
    printf("Failed to get local IP address: %s\n",
           get_nsapi_error_string(result));
    while (1);
  }

  printf("Connected to WLAN and got IP address %s\n", address.get_ip_address());


// Getting EPOCH time, then doing wifi_C++ so this only happends once

int wifi_c=0;
 while (wifi_c < 1) {
    TLSSocket socket;
    socket.set_timeout(500);
   result = socket.open(network);
    if (result != NSAPI_ERROR_OK) {
      printf("Failed to open TLSSocket: %s\n", get_nsapi_error_string(result));
      continue;
    }
    const char host[] = "api.ipgeolocation.io"; 
    result = network->gethostbyname(host, &address);
    if (result != NSAPI_ERROR_OK) {
      printf("Failed to get IP address of host %s: %s\n", host,
             get_nsapi_error_string(result));
      continue;
    }
    printf("IP address of server %s is %s\n", host, address.get_ip_address());
    address.set_port(443);
    result = socket.set_root_ca_cert(ipify_org_ca_root_certificate);
    if (result != NSAPI_ERROR_OK) {
      printf("Failed to set root certificate of the web site: %s\n",
             get_nsapi_error_string(result));
      continue;
    }
    result = socket.connect(address);
    if (result != NSAPI_ERROR_OK) {
      printf("Failed to connect to server at %s: %s\n", host,
             get_nsapi_error_string(result));
      continue;
    }
    printf("Successfully connected to server %s\n", host);

    const char request[] = "GET /timezone?apiKey=df544a98d46b48849674ed3dd607a210&tz=Europe/Oslo HTTP/1.1\r\n"
                           "Host: api.ipgeolocation.io\r\n"
                           "Connection: close\r\n"
                           "\r\n";

    result = send_request(&socket, request);

    if (result < 0) {
      printf("Failed to send request: %d\n", result);
      continue;
    }
    static char buffer[2000];
    result = read_response(&socket, buffer, sizeof(buffer));
    if (result < 0) {
      printf("Failed to read response: %d\n", result);
      continue;
    }
    char* json_begin = strchr(buffer, '{');
    char* json_end = strrchr(buffer, '}');
    if (json_begin == nullptr || json_end == nullptr) {
      printf("Failed to find JSON in response\n");
      continue;
    }
    json_end[1] = 0;
    printf("JSON response:\n");
    printf("%s\n", json_begin);
    json document = json::parse(json_begin);
    if (document.is_discarded()) {
      printf("The input is invalid JSON\n");
      continue;
    }
    int UnixTimeStamp;
    document["date_time_unix"].get_to(UnixTimeStamp);

    printf("Unix timer: %d\n", UnixTimeStamp);
      lcd.setCursor(0, 0 );
    lcd.printf("Unix epoch time:");
        lcd.setCursor(0, 1 );
    lcd.printf("%d\n", UnixTimeStamp);
    time_t epochconverted = document["date_time_unix"].get<int>()+7200; // CONVERT THE EPOCH
    set_time(epochconverted);
    thread_sleep_for(5000);
     lcd.clear();
     wifi_c++;

     thread_sleep_for(5000);
  }


 while (true){
    // This IS THE setup PART 
        time_t sec = time(NULL);
        char storedtime[32];
        char hourval2[4];
        char minval2[4];
        strftime(storedtime, 32, "%a %d %b %H:%M\n", localtime(&sec));
        strftime(hourval2, 2, "%H", localtime(&sec));
        strftime(minval2, 2, "%M", localtime(&sec));
        hournr2=stoi(hourval2);
        minnr2=stoi(minval2);

    // Here comes the if statements
    if (screen_selection == 1) {
            lcd.setCursor(0,0);
            lcd.printf("%s",storedtime);
             if (alarm==true){
                lcd.setCursor(0,1);
                lcd.printf("Alarm: %i:%i", hour, min);
            }
            else if (alarm==false){
                lcd.setCursor(0,1);
                lcd.printf("Alarm OFF: %i:%i",hour,min);

        }

    }
    
     else if (screen_selection == 2){
       lcd.setCursor(0,0);
            lcd.printf("Set alarm");
            if (increasehour.read()==1){
                hour+=1;
                if(hour==24){
                    hour=0;
                }
                lcd.setCursor(0, 1);
                lcd.printf("Alarm: %i:%i",hour,min);
            }
            if (increasemin.read()==1){
                min+=1;
                if(min==60){
                    min=0;
                }
                lcd.setCursor(0, 1);
                lcd.printf("Alarm: %i:%i",hour,min);
            }
        }
    

    else if (screen_selection == 3)
        {

           lcd.clear();
           humtemp_screen();
        }

        
   else if (screen_selection == 4)
        {
            lcd.clear();
            weather_screen();
        }


        else if (screen_selection == 5)
        {
            lcd.clear();
            weather_screen();
        }



    else if (screen_selection == 6) {
    lcd.clear();
    TCPSocket socket;
    socket.set_timeout(500);
    result = socket.open(network);
    if (result != NSAPI_ERROR_OK) {
      printf("Failed to open TCPSocket: %d\n", result);
      continue;
    }
    const char host[] = "feeds.bbci.co.uk";
    result = network->gethostbyname(host, &address);
    if (result != NSAPI_ERROR_OK) {
      printf("Failed to get IP address of host %s: %d\n", host, result);
      continue;
    }
    printf("IP address of server %s is %s\n", host, address.get_ip_address());
    address.set_port(80);
    result = socket.connect(address);
    if (result != NSAPI_ERROR_OK) {
      printf("Failed to connect to server at %s: %d\n", host, result);
      continue;
    }
    printf("Successfully connected to server %s\n", host);

    const char request[] = "GET /news/world/rss.xml HTTP/1.1\r\n"
                           "Host: feeds.bbci.co.uk\r\n"
                           "Connection: close\r\n"
                           "\r\n";
    result = send_request(&socket, request);
    if (result < 0) {
      printf("Failed to send request: %d\n", result);
      continue;
    }
    static constexpr size_t HTTP_RESPONSE_BUF_SIZE = 3000;
    static char response[HTTP_RESPONSE_BUF_SIZE];
    result = read_response(&socket, response, HTTP_RESPONSE_BUF_SIZE);
    if (result < 0) {
      printf("Failed to read response: %d\n", result);
      continue;
    }
    response[result] = '\0';

    std::vector<Match> matches = parse_data(response, "<title><![CDATA[", "]]></title>", result);

    std::string result;
    for (auto match = matches.begin() + 1; match < matches.end(); match++)
        result += *match;




uint32_t loop_cnt = 0;
const char * start = result.c_str();
const char * end = result.c_str()+ result.length();
while (start < end) {
lcd.setCursor(0, 0);
lcd.printf("%.*s ", (end - start > 16) ? 16
: end - start,
start);
start++; 
ThisThread::sleep_for((loop_cnt) ? 200ms : 1200ms);

loop_cnt++;
}
  }

   else if (screen_selection == 7) {
       lcd.clear();
       screen_selection = 1;
   }

        
        if(alarm==false) {
        buzzer.write(0);
        }

   if(hournr2==hour&&minnr2==min&&alarm==true){
            buzzer.write(1.0f);}

            if(snooze.read()==1){
                min+=5;
                 if(min==60){
                    min=0;
                    hour+1;
                } }



  
}
      };
    
      

// Sources: WIFI connection code taken from lectures and examples from Github
// Algorithm for finding relevant tags in a char array https://en.wikipedia.org/wiki/Rabin%E2%80%93Karp_algorithm#The_algorithm
// Rabin-Karp algorithm  
// Temperature and humdiity sensor code inspired from github examples
//Finaledit2checksave