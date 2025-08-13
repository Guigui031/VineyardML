#include "sd_card.h"

void connectSDCardModule() {
    SPI.begin(sck, miso, mosi, cs);
    if (!SD.begin(cs)) {
        Serial.println("SD Card Mount Failed");
        return;
    }
    Serial.println("SD Card Mount Succeeded");

    // Ensure parking_data folder exists
    if (!SD.exists(parking_data_folder)) {
        Serial.printf("Folder %s does not exist. Creating...\n", parking_data_folder);
        if (SD.mkdir(parking_data_folder)) {
            Serial.println("Folder created successfully.");
        } else {
            Serial.println("Failed to create folder.");
        }
    } else {
        Serial.printf("Folder %s already exists.\n", parking_data_folder);
    }
    
    // Ensure light_data folder exists
    if (!SD.exists(light_data_folder)) {
        Serial.printf("Folder %s does not exist. Creating...\n", light_data_folder);
        if (SD.mkdir(light_data_folder)) {
            Serial.println("Folder created successfully.");
        } else {
            Serial.println("Failed to create folder.");
        }
    } else {
        Serial.printf("Folder %s already exists.\n", light_data_folder);
    }
}

void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
    Serial.printf("Listing directory: %s\n", dirname);

    File root = fs.open(dirname);
    if (!root) {
        Serial.println("Failed to open directory");
        return;
    }
    if (!root.isDirectory()) {
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if (levels) {
                listDir(fs, file.path(), levels - 1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("  SIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

void createDir(fs::FS &fs, const char *path) {
    Serial.printf("Creating Dir: %s\n", path);
    if (fs.mkdir(path)) {
        Serial.println("Dir created");
    } else {
        Serial.println("mkdir failed");
    }
}

void removeDir(fs::FS &fs, const char *path) {
    Serial.printf("Removing Dir: %s\n", path);
    if (fs.rmdir(path)) {
        Serial.println("Dir removed");
    } else {
        Serial.println("rmdir failed");
    }
}

void readFile(fs::FS &fs, const char *path) {
    Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if (!file) {
        Serial.println("Failed to open file for reading");
        return;
    }

    Serial.print("Read from file: ");
    while (file.available()) {
        Serial.write(file.read());
    }
    file.close();
}

void writeFile(fs::FS &fs, const char *path, const char *message) {
    Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file for writing");
        return;
    }
    if (file.print(message)) {
        Serial.println("File written");
    } else {
        Serial.println("Write failed");
    }
    file.close();
}

void appendFile(fs::FS &fs, const char *path, const char *message) {
    Serial.printf("Appending to file: %s\n", path);

    File file = fs.open(path, FILE_APPEND);
    if (!file) {
        Serial.println("Failed to open file for appending");
        return;
    }
    if (file.print(message)) {
        Serial.println("Message appended");
    } else {
        Serial.println("Append failed");
    }
    file.close();
}

void renameFile(fs::FS &fs, const char *path1, const char *path2) {
    Serial.printf("Renaming file %s to %s\n", path1, path2);
    if (fs.rename(path1, path2)) {
        Serial.println("File renamed");
    } else {
        Serial.println("Rename failed");
    }
}

void deleteFile(fs::FS &fs, const char *path) {
    Serial.printf("Deleting file: %s\n", path);
    if (fs.remove(path)) {
        Serial.println("File deleted");
    } else {
        Serial.println("Delete failed");
    }
}

void testFileIO(fs::FS &fs, const char *path) {
    File file = fs.open(path);
    static uint8_t buf[512];
    size_t len = 0;
    uint32_t start = millis();
    uint32_t end = start;
    if (file) {
        len = file.size();
        size_t flen = len;
        start = millis();
        while (len) {
            size_t toRead = len;
            if (toRead > 512) {
                toRead = 512;
            }
            file.read(buf, toRead);
            len -= toRead;
        }
        end = millis() - start;
        Serial.printf("%u bytes read for %lu ms\n", flen, end);
        file.close();
    } else {
        Serial.println("Failed to open file for reading");
    }

    file = fs.open(path, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file for writing");
        return;
    }

    size_t i;
    start = millis();
    for (i = 0; i < 2048; i++) {
        file.write(buf, 512);
    }
    end = millis() - start;
    Serial.printf("%u bytes written for %lu ms\n", 2048 * 512, end);
    file.close();
}

void logEvent(int value) {
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to get time");
        return;
    }

    // Create filename: /YYYY-MM-DD.csv
    char filename[40];
    strftime(datestr, sizeof(datestr), "%Y-%m-%d", &timeinfo);
    snprintf(filename, sizeof(filename), "%s/%s.csv", parking_data_folder, datestr);
    Serial.print("Creating/using file: ");
    Serial.println(filename);

    // Create header if file doesn't exist
    if (!SD.exists(filename)) {
        writeFile(SD, filename, "index,date,time,distance(cm),millis,notification\n");
    }

    // Create data line
    char line[64];
    strftime(timestr, sizeof(timestr), "%H:%M:%S", &timeinfo);
    snprintf(line, sizeof(line), "%d,%s,%s,%d,%d,%d\n", value, datestr, timestr, cm, lastEventTime, notificationsEnabled);

    // Append to file
    appendFile(SD, filename, line);

    // Print to Serial
    Serial.print("Logged: ");
    Serial.print(line);
}

void logLightValue(int value, bool from_command) {
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to get time");
        return;
    }

    // Create filename: /YYYY-MM-DD.csv
    char filename[40];
    strftime(dateDatastr, sizeof(dateDatastr), "%Y-%m-%d", &timeinfo);
    snprintf(filename, sizeof(filename), "%s/%s.csv", light_data_folder, dateDatastr);
    Serial.print("Creating/using file: ");
    Serial.println(filename);

    // Create header if file doesn't exist
    if (!SD.exists(filename)) {
        writeFile(SD, filename, "index,date,time,light,millis,command\n");
    }

    // Create data line
    char line[64];
    strftime(timeDatastr, sizeof(timeDatastr), "%H:%M:%S", &timeinfo);
    snprintf(line, sizeof(line), "%d,%s,%s,%d,%d,%d\n", value, dateDatastr, timeDatastr, lightValue, lastLogTime, from_command);

    // Append to file
    appendFile(SD, filename, line);

    // Print to Serial
    Serial.print("Logged: ");
    Serial.print(line);
}