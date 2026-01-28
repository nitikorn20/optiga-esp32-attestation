# Build and Flash (ESP-IDF)

Use the ESP-IDF CLI to build and flash this project.

```bash
idf.py set-target esp32
idf.py build
idf.py -p COM7 flash
idf.py monitor
```

**Serial settings:** 115200 baud, 8N1, no flow control.
