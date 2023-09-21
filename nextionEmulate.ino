#include <NeoICSerial.h>

bool bdebug = false;

NeoICSerial emulateNextion;
int ARDUINOSERIALBUFFERSIZE = 64;
#define NEXTION_BAUD_DEFAULT 9600
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
}
const int sequence_length = 3;
const int command_length = 80;
byte sequence[sequence_length] = {0xFF, 0xFF, 0xFF}; // The sequence to look for
byte received[sequence_length]; // The last 3 received bytes
char command[command_length + 1]; // The command string, +1 for null termination
int received_index = 0; // The index to store the next received byte
int command_index = 0; // The index to store the next command character

long filesize;
long baudrate;

bool  downloadmode = false;

long total_bytes_read = 0;

bool nextioninitdone = false;
int chunk = 0;
void loop() {
    if (downloadmode) {
       long bytes_to_read = min(filesize - total_bytes_read, 4096);
       long bytes_read = 0;
       
       unsigned long timeout = 30000L;
       unsigned long tstart = millis();
       for (long i = 0; i < bytes_to_read; i++) {
           int avail = 0;
           do {
               avail = emulateNextion.available();
               if (avail == 0) delay(20);
           } while (avail == 0);

         
           if (avail >=ARDUINOSERIALBUFFERSIZE - 1) { Serial.print(" Serial buffer overflow:  avail="); Serial.println(avail); }

           while (avail > 0) {
               char c = emulateNextion.read();
               bytes_read++;
               avail--;
               if (bytes_read == bytes_to_read) {
                   emulateNextion.write(0x05); // acknowledge
                   Serial.print("write(0x05), ");
                   Serial.print(bytes_read);
                   Serial.println("bytes read.");
                   goto received;
               }
              
           }

           if (millis() - tstart > timeout) {
               Serial.println("timeout");
               goto received;
           }
           
       }
      
   received:
       total_bytes_read += bytes_read;
       Serial.print("total_bytes_read="); Serial.println(total_bytes_read);
       if (total_bytes_read >= filesize) {
           nextioninitdone = false;
           downloadmode = false;
           Serial.println("Upload finished");
       }
    }
    else {
        if (!nextioninitdone) {
            nextioninitdone = true;
            emulateNextion.begin(NEXTION_BAUD_DEFAULT);
            Serial.print("Nextion display emulation, waiting for cmd.");
            Serial.print(NEXTION_BAUD_DEFAULT);
            Serial.println(" baud.");
            memset(command, 0, sizeof(command));
        }
         if (emulateNextion.available()) {
                byte data = emulateNextion.read(); // Read the incoming byte
               //Store the byte as part of the command if it's within range
                if (command_index < command_length) {
                    command[command_index++] = data;
                    command[command_index] = '\0'; // Null-terminate the command string
                }
                // Also store the byte in the sequence buffer
                received[received_index] = data;
                received_index = (received_index + 1) % sequence_length;
                // Check if the received bytes match the sequence
                boolean sequence_matched = true;
                for (int i = 0; i < sequence_length; i++) {
                    if (sequence[i] != received[(received_index + i) % sequence_length]) {
                        sequence_matched = false;
                        break;
                    }
                }

                if (sequence_matched) {
                    // remove Oxff0xff0xff bytes from command:
                   
                    memset(received, 0, sizeof(received));
                    received_index = 0; // The index to store the next received byte
                    command_index = 0; // The index to store the next command character
                    int l = strlen(command);
                      command[l - 3] = 0;
                   
                    String tmp = "Command: [";
                    tmp+=String(command);
                    tmp += "] .";
                    Serial.println(tmp);
                    Serial.flush();
                    if (!strncmp(command, "uploading", 9)) {
                        emulateNextion.write(0x05); // acknowledge
                    }
                    if (!strcmp(command, "uploaded.")) {
                        emulateNextion.write(0x05); // acknowledge
                        nextioninitdone = false;
                    }
                    // Handle the event here
                    if (!strcmp(command, "connect")) {
                        emulateNextion.write("comok 1,101,NX4024T032_011R,52,61488,D264B8204F0E1828,16777216");
                        emulateNextion.write(0x05); // acknowledge
                    }
                    if (sscanf(command, "whmi-wri %ld,%ld,0", &filesize, &baudrate) == 2) {

                        memset(command, 0, sizeof(command));
                        Serial.print("filesize=");
                        Serial.println(filesize);
                        
                        Serial.print("Nextion display emulation, baud=");
                        Serial.println(baudrate);
                        emulateNextion.begin(baudrate);
                        delay(50);
                        emulateNextion.write(0x05); // acknowledge
                        total_bytes_read = 0;
                        if(!bdebug)  downloadmode = true;
                        return;
                    }
                    memset(command, 0, sizeof(command));
                }
            }
    }
}

