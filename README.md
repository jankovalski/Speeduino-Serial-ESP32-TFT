# Speeduino-Serial-ESP32-TFT

Comms code is from https://github.com/MielArrojado/speeduino-ardugauge

![screen](https://user-images.githubusercontent.com/46295168/162586650-f39137a9-696a-4391-805e-a7798a732ed7.jpg)

Sync loss counter is not exposed on Serial3 by default. I added it to cancomms.ino manually: ```fullStatus[122] = currentStatus.syncLossCounter;``` also increased ```NEW_CAN_PACKET_SIZE``` in cancomms.h from 122 to 123. If you don't need that, you might want to change ```DATA_LEN``` in Comms.h of this project back to 122 and remove fetching this value from main ino file.
