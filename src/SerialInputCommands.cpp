#include "SerialCommands.h"
#include "sonos_controller.h"

// Test content definitions
const SerialCommands::TestContent SerialCommands::TEST_TRACKS[] = {
    {"spotify:track:3iHzKA9HlXf5wsGdsrsnSA", "Test Track 1"},
    {"spotify:track:7GhIk7Il098yCjg4BQjzvb", "Test Track 2"},  // Hey Jude
    {"spotify:track:4uLU6hMCjMI75M1A2tKUQC", "Test Track 3"},  // Imagine
    {"spotify:track:2lfPecqFbH8X4lHSpTxt8l", "Test Track 4"},  // Bohemian Rhapsody
    {"spotify:track:5Z01UMMf7V1o0MzF86s6WJ", "Test Track 5"}   // Stairway to Heaven
};

const SerialCommands::TestContent SerialCommands::TEST_ALBUMS[] = {
    {"spotify:album:1DFixLWuPkv3KT3TnV35m3", "Test Album 1"},
    {"spotify:album:3kTZ0JkYfS8hpCJpMGhT8g", "Test Album 2"},  // Abbey Road
    {"spotify:album:6QaVfG1pHYl1z15ZxkvVDW", "Test Album 3"}   // Thriller
};

const SerialCommands::TestContent SerialCommands::TEST_PLAYLISTS[] = {
    {"spotify:playlist:37i9dQZEVXbMDoHDwVN2tF", "Test Playlist 1"},
    {"spotify:playlist:37i9dQZF1DXcBWIGoYBM5M", "Test Playlist 2"},  // Today's Top Hits
    {"spotify:playlist:37i9dQZF1DX5Q27plkaOQ3", "Test Playlist 3"}   // Classic Rock
};

SerialCommands::SerialCommands(SonosController& sonosRef)
    : sonos(sonosRef) {
    inputBuffer.reserve(256);
}

void SerialCommands::begin() {
    Serial.println("\n=== SONOS CONTROLLER SERIAL COMMANDS ===");
    help();
    Serial.println("Enter 'help' for command list");
    Serial.println("=======================================\n");
}

void SerialCommands::handleInput() {
    while (Serial.available()) {
        char c = Serial.read();
        if (c == '\n') {
            if (inputBuffer.length() > 0) {
                processCommand(inputBuffer);
                inputBuffer = "";
            }
        } else if (c == '\r') {
            // Ignore carriage return
        } else if (c == '\b' || c == 127) {  // Handle backspace/delete
            if (inputBuffer.length() > 0) {
                inputBuffer.remove(inputBuffer.length() - 1);
                Serial.print("\b \b");  // Clear the character
            }
        } else if (c >= 32 && c <= 126) {  // Printable characters
            inputBuffer += c;
            Serial.print(c);  // Echo
        }
    }
}

void SerialCommands::processCommand(const String& command) {
    Serial.println();  // New line after command
    
    int spaceIndex = command.indexOf(' ');
    if (spaceIndex > 0) {
        String cmd = command.substring(0, spaceIndex);
        String args = command.substring(spaceIndex + 1);
        processCommandWithArgs(cmd, args);
    } else {
        processCommandWithArgs(command, "");
    }
}

void SerialCommands::processCommandWithArgs(const String& cmd, const String& args) {
    String cmdLower = cmd;
    cmdLower.toLowerCase();
    
    if (cmdLower == "help" || cmdLower == "?") {
        help();
    } else if (cmdLower == "test") {
        testAlbumFromCapture();
    } else if (cmdLower == "testradio") {
        playTestRadio();
    } else if (cmdLower == "testspotify") {
        playTestSpotify();
    } else if (cmdLower == "rawpos") {
        rawpos();
    } else if (cmdLower == "spotifytest") {
        spotifyTest();
    } else if (cmdLower == "queuetest") {
        queueTest();
    } else if (cmdLower == "spotifyalbumtest") {
        spotifyAlbumTest();
    } else if (cmdLower == "testfile") {
        playTestLocalFile();
    } else if (cmdLower == "play") {
        processPlayCommand(args);
    } else if (cmdLower == "services") {
        listServices();
    } else if (cmdLower == "info") {
        getCurrentInfo();
    } else if (cmdLower == "uri") {
        getCurrentURI();
    } else if (cmdLower == "transport") {
        getTransportInfo();
    } else if (cmdLower == "media") {
        getMediaInfo();
    } else if (cmdLower == "memory") {
        debugMemory();
    } else if (cmdLower == "testqueue") {
        testQueueLoad();
    } else if (cmdLower == "playqueue") {
        int index = args.toInt();
        if (index > 0) {
            playQueueItem(index);
        } else {
            Serial.println("Usage: playqueue [track_number]");
        }
    } else if (cmdLower == "groups") {
        getGroupInfo();
    } else if (cmdLower == "joingroup") {
        processGroupCommand(args);
    } else if (cmdLower == "leavegroup") {
        leaveAllGroups();
    } else if (cmdLower == "clearqueue") {
        clearQueue();
    } else if (cmdLower == "debug") {
        sonos.debugCurrentPlayback();
    } else {
        Serial.println("Unknown command. Type 'help' for list.");
    }
}

void SerialCommands::processPlayCommand(const String& args) {
    if (args.length() == 0) {
        Serial.println("Usage:");
        Serial.println("  play track [1-5]  - Play test track");
        Serial.println("  play album [1-3]  - Play test album");
        Serial.println("  play playlist [1-3] - Play test playlist");
        Serial.println("  play spotify [uri] [name] - Play custom Spotify");
        return;
    }
    
    int spaceIndex = args.indexOf(' ');
    String type = args.substring(0, spaceIndex);
    String rest = (spaceIndex > 0) ? args.substring(spaceIndex + 1) : "";
    
    type.toLowerCase();
    
    if (type == "track") {
        int trackNum = rest.toInt();
        if (trackNum >= 1 && trackNum <= NUM_TEST_TRACKS) {
            playSpotifyTrack(TEST_TRACKS[trackNum-1].uri, TEST_TRACKS[trackNum-1].name);
        } else {
            Serial.printf("Track number must be 1-%d\n", NUM_TEST_TRACKS);
        }
    } else if (type == "album") {
        int albumNum = rest.toInt();
        if (albumNum >= 1 && albumNum <= NUM_TEST_ALBUMS) {
            playSpotifyAlbum(TEST_ALBUMS[albumNum-1].uri, TEST_ALBUMS[albumNum-1].name);
        } else {
            Serial.printf("Album number must be 1-%d\n", NUM_TEST_ALBUMS);
        }
    } else if (type == "playlist") {
        int playlistNum = rest.toInt();
        if (playlistNum >= 1 && playlistNum <= NUM_TEST_PLAYLISTS) {
            playSpotifyPlaylist(TEST_PLAYLISTS[playlistNum-1].uri, TEST_PLAYLISTS[playlistNum-1].name);
        } else {
            Serial.printf("Playlist number must be 1-%d\n", NUM_TEST_PLAYLISTS);
        }
    } else if (type == "spotify") {
        int firstSpace = rest.indexOf(' ');
        if (firstSpace > 0) {
            String uri = rest.substring(0, firstSpace);
            String name = rest.substring(firstSpace + 1);
            playSpotifyTrack(uri, name);
        } else {
            Serial.println("Usage: play spotify [spotify:track:xxx] [Track Name]");
        }
    } else {
        Serial.println("Invalid play type. Use: track, album, playlist, or spotify");
    }
}

void SerialCommands::processGroupCommand(const String& args) {
    if (args.length() == 0) {
        Serial.println("Usage: joingroup [coordinator_index] [device_index]");
        Serial.println("Example: joingroup 0 1  (Group device 1 to coordinator 0)");
        return;
    }
    
    int spaceIndex = args.indexOf(' ');
    if (spaceIndex <= 0) {
        Serial.println("Invalid format. Use: joingroup [coordinator] [device]");
        return;
    }
    
    String coordStr = args.substring(0, spaceIndex);
    String deviceStr = args.substring(spaceIndex + 1);
    
    int coordIndex = coordStr.toInt();
    int deviceIndex = deviceStr.toInt();
    
    if (sonos.joinGroup(deviceIndex, coordIndex)) {
        Serial.println("✓ Group joined successfully");
    } else {
        Serial.println("✗ Failed to join group");
    }
}

void SerialCommands::help() {
    Serial.println("\n=== SONOS COMMANDS ===");
    Serial.println("help                - Show this help");
    Serial.println("test                - Test connection to current device");
    Serial.println("info                - Show current playback info");
    Serial.println("uri                 - Show current URI");
    Serial.println("transport           - Show transport info");
    Serial.println("media               - Show media info");
    
    Serial.println("\n=== PLAYBACK TESTS ===");
    Serial.println("testradio           - Test radio stream");
    Serial.println("testspotify         - Test Spotify (legacy)");
    Serial.println("spotifytest         - Spotify track test (direct URI)");
    Serial.println("queuetest           - Spotify track via queue (better metadata)");
    Serial.println("spotifyalbumtest    - Spotify album via container URI");
    Serial.println("testfile            - Test local file (if available)");
    Serial.println("play track [1-5]    - Play test track");
    Serial.println("play album [1-3]    - Play test album");
    Serial.println("play playlist [1-3] - Play test playlist");
    Serial.println("play spotify [uri] [name] - Play custom Spotify");
    
    Serial.println("\n=== QUEUE ===");
    Serial.println("testqueue           - Load test queue");
    Serial.println("playqueue [n]       - Play queue item n");
    Serial.println("clearqueue          - Clear queue");
    
    Serial.println("\n=== GROUPS ===");
    Serial.println("groups              - Show group info");
    Serial.println("joingroup [c] [d]   - Join device d to coordinator c");
    Serial.println("leavegroup          - Leave all groups");
    
    Serial.println("\n=== SYSTEM ===");
    Serial.println("services            - List available services");
    Serial.println("memory              - Show memory usage");
    Serial.println("debug               - Debug current playback");
    Serial.println("=========================\n");
}

void SerialCommands::testConnection() {
    Serial.println("\n=== TESTING CONNECTION ===");
    
    SonosDevice* dev = sonos.getCurrentDevice();
    if (!dev) {
        Serial.println("✗ No device selected");
        return;
    }
    
    Serial.printf("Device: %s\n", dev->roomName.c_str());
    Serial.printf("IP: %s\n", dev->ip.toString().c_str());
    Serial.printf("Connected: %s\n", dev->connected ? "YES" : "NO");
    
    // Try a simple SOAP call
    Serial.println("Pinging device...");
    
    // Use GetVolume as a simple test
    String resp = sonos.sendSOAP("RenderingControl", "GetVolume", 
        "<InstanceID>0</InstanceID><Channel>Master</Channel>");
    
    if (resp.length() > 0 && resp.indexOf("Fault") < 0) {
        Serial.println("✓ Device is responding");
        String vol = sonos.extractXML(resp, "CurrentVolume");
        Serial.printf("Current volume: %s\n", vol.c_str());
    } else {
        Serial.println("✗ Device not responding");
    }
    
    Serial.println("=== END TEST ===\n");
}

void SerialCommands::playTestRadio() {
    Serial.println("\n=== TESTING RADIO PLAYBACK ===");
    
    const char* radioUrl = "http://icecast.thisisdax.com/ClassicFMMP3";
    const char* radioMeta = R"EOF(<DIDL-Lite xmlns:dc="http://purl.org/dc/elements/1.1/" xmlns:upnp="urn:schemas-upnp-org:metadata-1-0/upnp/" xmlns:r="urn:schemas-rinconnetworks-com:metadata-1-0/" xmlns="urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/"><item id="100" parentID="0" restricted="false"><dc:title>Classic FM</dc:title><upnp:class>object.item.audioItem.audioBroadcast</upnp:class><res protocolInfo="http-get:*:audio/mpeg:*">http://icecast.thisisdax.com/ClassicFMMP3</res></item></DIDL-Lite>)EOF";
    
    Serial.printf("Playing radio: %s\n", radioUrl);
    
    bool success = sonos.playURI(radioUrl, radioMeta);
    
    if (success) {
        Serial.println("✓ Radio playback started");
        delay(3000);
        getCurrentInfo();
    } else {
        Serial.println("✗ Radio playback failed");
    }
    
    Serial.println("=== END TEST ===\n");
}

void SerialCommands::playTestSpotify() {
    Serial.println("\n=== TESTING SPOTIFY PLAYBACK ===");
    
    // First check if Spotify is available
    Serial.println("Checking Spotify service...");
    sonos.checkSpotifyService();
    
    delay(1000);
    
    // Try playing a test track
    Serial.println("Playing test track...");
    bool success = sonos.playSpotifyTrack(TEST_TRACKS[0].uri, TEST_TRACKS[0].name);
    
    if (success) {
        Serial.println("✓ Spotify playback started");
        delay(3000);
        getCurrentInfo();
    } else {
        Serial.println("✗ Spotify playback failed");
        Serial.println("Try running 'services' command to see available services");
    }
    
    Serial.println("=== END TEST ===\n");
}

void SerialCommands::playTestLocalFile() {
    Serial.println("\n=== TESTING LOCAL FILE PLAYBACK ===");
    
    // Modify this URI to match your network share
    const char* testUri = "x-file-cifs://SERVER/Music/test.mp3";
    const char* testMeta = R"EOF(<DIDL-Lite xmlns:dc="http://purl.org/dc/elements/1.1/" xmlns:upnp="urn:schemas-upnp-org:metadata-1-0/upnp/" xmlns:r="urn:schemas-rinconnetworks-com:metadata-1-0/" xmlns="urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/"><item id="1" parentID="0" restricted="false"><dc:title>Test Local File</dc:title><upnp:class>object.item.audioItem.musicTrack</upnp:class><res protocolInfo="http-get:*:audio/mpeg:*">x-file-cifs://SERVER/Music/test.mp3</res></item></DIDL-Lite>)EOF";
    
    Serial.printf("Playing local file: %s\n", testUri);
    Serial.println("Note: Update the URI to match your network share");
    
    bool success = sonos.playURI(testUri, testMeta);
    
    if (success) {
        Serial.println("✓ Local file playback attempted (check if file exists)");
        delay(3000);
        getCurrentInfo();
    } else {
        Serial.println("✗ Local file playback failed");
    }
    
    Serial.println("=== END TEST ===\n");
}

void SerialCommands::playSpotifyTrack(const String& uri, const String& name) {
    Serial.printf("\n=== PLAYING SPOTIFY TRACK ===\n");
    Serial.printf("URI: %s\n", uri.c_str());
    Serial.printf("Name: %s\n", name.c_str());
    
    bool success = sonos.playSpotifyTrack(uri, name);
    
    if (success) {
        Serial.println("✓ Spotify track playback started");
        delay(2000);
        getCurrentInfo();
    } else {
        Serial.println("✗ Failed to play Spotify track");
        Serial.println("Trying queue method...");
        
        success = sonos.playSpotifyTrackViaQueue(uri, name);
        if (success) {
            Serial.println("✓ Queue method succeeded");
            delay(2000);
            getCurrentInfo();
        } else {
            Serial.println("✗ Both methods failed");
        }
    }
    
    Serial.println("=== END ===\n");
}

void SerialCommands::playSpotifyAlbum(const String& uri, const String& name) {
    Serial.printf("\n=== PLAYING SPOTIFY ALBUM ===\n");
    Serial.printf("URI: %s\n", uri.c_str());
    Serial.printf("Name: %s\n", name.c_str());
    
    // Extract album ID
    String albumId = uri;
    if (uri.startsWith("spotify:album:")) {
        albumId = uri.substring(14);
    }
    
    // Format for Sonos
    String sonosUri = "x-sonos-spotify:spotify:album:" + albumId;
    
    // Album metadata
    String metadata = R"EOF(<DIDL-Lite xmlns="urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/" 
        xmlns:dc="http://purl.org/dc/elements/1.1/" 
        xmlns:upnp="urn:schemas-upnp-org:metadata-1-0/upnp/" 
        xmlns:r="urn:schemas-rinconnetworks-com:metadata-1-0/">
        <container id="1" parentID="0" restricted="false">
        <dc:title>)" + name + R"EOF(</dc:title>
        <upnp:class>object.container.album.musicAlbum</upnp:class>
        <desc id="cdudn" nameSpace="urn:schemas-rinconnetworks-com:metadata-1-0/">SA_RINCON12_X_#Svc12-5-Token</desc>
        </container>
        </DIDL-Lite>)EOF";
    
    bool success = sonos.playURI(sonosUri.c_str(), metadata.c_str());
    
    if (success) {
        Serial.println("✓ Spotify album playback started");
        delay(2000);
        getCurrentInfo();
    } else {
        Serial.println("✗ Failed to play Spotify album");
    }
    
    Serial.println("=== END ===\n");
}

void SerialCommands::playSpotifyPlaylist(const String& uri, const String& name) {
    Serial.printf("\n=== PLAYING SPOTIFY PLAYLIST ===\n");
    Serial.printf("URI: %s\n", uri.c_str());
    Serial.printf("Name: %s\n", name.c_str());
    
    // Extract playlist ID
    String playlistId = uri;
    if (uri.startsWith("spotify:playlist:")) {
        playlistId = uri.substring(17);
    }
    
    // Format for Sonos
    String sonosUri = "x-sonos-spotify:spotify:playlist:" + playlistId;
    
    // Playlist metadata
    String metadata = R"EOF(<DIDL-Lite xmlns="urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/" 
        xmlns:dc="http://purl.org/dc/elements/1.1/" 
        xmlns:upnp="urn:schemas-upnp-org:metadata-1-0/upnp/" 
        xmlns:r="urn:schemas-rinconnetworks-com:metadata-1-0/">
        <container id="1" parentID="0" restricted="false">
        <dc:title>)" + name + R"EOF(</dc:title>
        <upnp:class>object.container.playlistContainer</upnp:class>
        <desc id="cdudn" nameSpace="urn:schemas-rinconnetworks-com:metadata-1-0/">SA_RINCON12_X_#Svc12-5-Token</desc>
        </container>
        </DIDL-Lite>)EOF";
    
    bool success = sonos.playURI(sonosUri.c_str(), metadata.c_str());
    
    if (success) {
        Serial.println("✓ Spotify playlist playback started");
        delay(2000);
        getCurrentInfo();
    } else {
        Serial.println("✗ Failed to play Spotify playlist");
    }
    
    Serial.println("=== END ===\n");
}

void SerialCommands::clearQueue() {
    Serial.println("\n=== CLEARING QUEUE ===");
    
    String resp = sonos.sendSOAP("AVTransport", "RemoveAllTracksFromQueue", "<InstanceID>0</InstanceID>");
    
    if (resp.length() > 0 && resp.indexOf("Fault") < 0) {
        Serial.println("✓ Queue cleared");
    } else {
        Serial.println("✗ Failed to clear queue");
    }
    
    Serial.println("=== END ===\n");
}

void SerialCommands::listServices() {
    Serial.println("\n=== AVAILABLE MUSIC SERVICES ===");
    String services = sonos.listMusicServices();
    Serial.println(services);
    Serial.println("=== END ===\n");
}

void SerialCommands::getCurrentInfo() {
    Serial.println("\n=== CURRENT PLAYBACK INFO ===");
    sonos.debugCurrentPlayback();
    
    // Also get transport info
    getTransportInfo();
    getMediaInfo();
    
    Serial.println("=== END ===\n");
}

void SerialCommands::getCurrentURI() {
    Serial.println("\n=== CURRENT URI ===");
    sonos.getCurrentURI();
    Serial.println("=== END ===\n");
}

// Dump raw GetPositionInfo XML so we can see the exact DIDL the native app sends
void SerialCommands::rawpos() {
    Serial.println("\n=== RAW GetPositionInfo ===");
    String resp = sonos.sendSOAP("AVTransport", "GetPositionInfo", "<InstanceID>0</InstanceID>");
    if (resp.length() == 0) {
        Serial.println("No response");
    } else {
        // Print in chunks to avoid serial truncation
        Serial.printf("Response (%d chars):\n", resp.length());
        for (int i = 0; i < (int)resp.length(); i += 200) {
            Serial.printf("%.200s", resp.c_str() + i);
        }
        Serial.println();
    }
    Serial.println("=== END ===\n");
}

void SerialCommands::getTransportInfo() {
    Serial.println("\n=== TRANSPORT INFO ===");
    
    String resp = sonos.sendSOAP("AVTransport", "GetTransportInfo", "<InstanceID>0</InstanceID>");
    
    if (resp.length() > 0) {
        if (resp.indexOf("Fault") >= 0) {
            Serial.println("Error getting transport info");
        } else {
            Serial.println("Transport Info Response:");
            Serial.println(resp);
        }
    } else {
        Serial.println("No response");
    }
    
    Serial.println("=== END ===\n");
}

void SerialCommands::getMediaInfo() {
    Serial.println("\n=== MEDIA INFO ===");
    
    String resp = sonos.sendSOAP("AVTransport", "GetMediaInfo", "<InstanceID>0</InstanceID>");
    
    if (resp.length() > 0) {
        if (resp.indexOf("Fault") >= 0) {
            Serial.println("Error getting media info");
        } else {
            // Parse and display key info
            String currentURI = sonos.extractXML(resp, "CurrentURI");
            String currentMeta = sonos.extractXML(resp, "CurrentURIMetaData");
            currentMeta = sonos.decodeHTML(currentMeta);
            
            Serial.printf("Current URI: %s\n", currentURI.c_str());
            Serial.println("Current Metadata:");
            Serial.println(currentMeta);
            
            // Extract station name for radio
            if (currentMeta.indexOf("dc:title") > 0) {
                String title = sonos.extractXML(currentMeta, "dc:title");
                if (title.length() > 0) {
                    Serial.printf("Title: %s\n", title.c_str());
                }
            }
        }
    } else {
        Serial.println("No response");
    }
    
    Serial.println("=== END ===\n");
}

void SerialCommands::debugMemory() {
    Serial.println("\n=== MEMORY USAGE ===");
    
    size_t freeHeap = heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    size_t freeDMA = heap_caps_get_free_size(MALLOC_CAP_DMA);
    size_t freeSPIRAM = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
    
    Serial.printf("Free Heap: %u bytes (%.1f KB)\n", (unsigned)freeHeap, freeHeap / 1024.0);
    Serial.printf("Free DMA:  %u bytes (%.1f KB)\n", (unsigned)freeDMA, freeDMA / 1024.0);
    
    if (freeSPIRAM > 0) {
        Serial.printf("Free SPI RAM: %u bytes (%.1f KB)\n", (unsigned)freeSPIRAM, freeSPIRAM / 1024.0);
    }
    
    Serial.printf("Total Heap: %u bytes\n", (unsigned)heap_caps_get_total_size(MALLOC_CAP_DEFAULT));
    Serial.printf("Largest Free Block: %u bytes\n", (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_DEFAULT));
    
    Serial.println("=== END ===\n");
}

void SerialCommands::testQueueLoad() {
    Serial.println("\n=== TESTING QUEUE LOAD ===");
    
    SonosDevice* dev = sonos.getCurrentDevice();
    if (!dev) {
        Serial.println("No device selected");
        return;
    }
    
    Serial.println("Loading queue...");
    bool success = sonos.updateQueue(0);
    
    if (success) {
        Serial.printf("Queue loaded: %d tracks\n", dev->totalTracks);
        Serial.printf("Queue window: %d items\n", dev->queueSize);
        
        for (int i = 0; i < dev->queueSize && i < 5; i++) {
            Serial.printf("%2d. %s - %s\n", 
                i + 1,
                dev->queue[i].title.c_str(),
                dev->queue[i].artist.c_str());
        }
        
        if (dev->queueSize > 5) {
            Serial.printf("... and %d more\n", dev->queueSize - 5);
        }
    } else {
        Serial.println("Failed to load queue");
    }
    
    Serial.println("=== END ===\n");
}

void SerialCommands::playQueueItem(int index) {
    Serial.printf("\n=== PLAYING QUEUE ITEM %d ===\n", index);
    
    CommandRequest_t cmd = {CMD_PLAY_QUEUE_ITEM, index};
    xQueueSend(sonos.getCommandQueue(), &cmd, portMAX_DELAY);
    
    Serial.println("Command sent to queue");
    delay(1000);
    getCurrentInfo();
    
    Serial.println("=== END ===\n");
}

void SerialCommands::getGroupInfo() {
    Serial.println("\n=== GROUP INFORMATION ===");
    
    sonos.updateGroupInfo();
    
    SonosDevice* dev = sonos.getCurrentDevice();
    if (dev) {
        Serial.printf("Current device: %s\n", dev->roomName.c_str());
        Serial.printf("Group coordinator: %s\n", dev->isGroupCoordinator ? "YES" : "NO");
        Serial.printf("Group UUID: %s\n", dev->groupCoordinatorUUID.c_str());
        Serial.printf("Group members: %d\n", dev->groupMemberCount);
    }
    
    int deviceCount = sonos.getDeviceCount();
    Serial.printf("\nTotal devices: %d\n", deviceCount);
    
    for (int i = 0; i < deviceCount; i++) {
        SonosDevice* device = sonos.getDevice(i);
        Serial.printf("\n[%d] %s\n", i, device->roomName.c_str());
        Serial.printf("  IP: %s\n", device->ip.toString().c_str());
        Serial.printf("  Coordinator: %s\n", device->isGroupCoordinator ? "YES" : "NO");
        if (device->isGroupCoordinator) {
            Serial.printf("  Members: %d\n", device->groupMemberCount);
        } else {
            Serial.printf("  Following: %s\n", device->groupCoordinatorUUID.c_str());
        }
    }
    
    Serial.println("=== END ===\n");
}

void SerialCommands::joinGroups() {
    Serial.println("\n=== JOINING GROUPS ===");
    Serial.println("Note: Use 'joingroup [coordinator] [device]' to join specific devices");
    Serial.println("Example: joingroup 0 1");
    Serial.println("=== END ===\n");
}

void SerialCommands::leaveAllGroups() {
    Serial.println("\n=== LEAVING ALL GROUPS ===");

    int deviceCount = sonos.getDeviceCount();
    for (int i = 0; i < deviceCount; i++) {
        Serial.printf("Device %d: ", i);
        if (sonos.leaveGroup(i)) {
            Serial.println("LEFT group");
        } else {
            Serial.println("Failed or already standalone");
        }
        delay(100);
    }

    Serial.println("=== END ===\n");
}

void SerialCommands::spotifyTest() {
    Serial.println("\n=== SPOTIFY TRACK TEST ===");

    const char* TRACK_ID   = "3iHzKA9HlXf5wsGdsrsnSA";

    SonosDevice* dev = sonos.getCurrentDevice();
    if (!dev) {
        Serial.println("ERROR: No device selected. Run discovery first.");
        Serial.println("=== END ===\n");
        return;
    }
    Serial.printf("Device : %s  IP: %s\n", dev->roomName.c_str(), dev->ip.toString().c_str());

    // ── Step 1: Discover Spotify service ID ──────────────────────────────────
    Serial.println("\n[1] Discovering Spotify service ID...");
    int discoveredSid = sonos.getSpotifyServiceId();
    if (discoveredSid > 0) {
        Serial.printf("    Found: sid=%d\n", discoveredSid);
    } else {
        Serial.println("    Not found — defaulting to 12");
        discoveredSid = 12;
    }

    // ── Step 2: Discover the linked account sn via GetSessionId ──────────────
    Serial.println("\n[2] Checking Spotify account link (GetSessionId)...");
    int linkedSn = sonos.getSpotifyAccountSn();
    if (linkedSn > 0) {
        Serial.printf("    Account linked, sn=%d confirmed\n", linkedSn);
    } else {
        Serial.println("    GetSessionId failed — Spotify may not be linked");
        Serial.println("    Will try sn=1..5 anyway");
    }

    // ── Step 3: Try combinations of SID and account number (sn) ─────────────
    // sn is the per-account service number; start with GetSessionId result if known,
    // otherwise sweep 1-5.
    int sidCandidates[] = {discoveredSid, 0};
    // sn=5 confirmed correct for this device (observed from native Sonos app).
    // Put it first, then any GetSessionId result, then remaining values.
    int snCandidates[7];
    int snCount = 0;
    snCandidates[snCount++] = 5;  // confirmed
    if (linkedSn > 0 && linkedSn != 5) {
        snCandidates[snCount++] = linkedSn;
    }
    for (int s = 1; s <= 5; s++) {
        if (s != linkedSn) snCandidates[snCount++] = s;
    }
    snCandidates[snCount] = 0;

    Serial.println("\n[3] Attempting playback...");

    for (int si = 0; sidCandidates[si] != 0; si++) {
        int curSid = sidCandidates[si];
        for (int sni = 0; snCandidates[sni] != 0; sni++) {
            int curSn = snCandidates[sni];

            // Build Sonos Spotify URI (colons must be %-encoded).
            // & in query params must be &amp; — this goes inside XML element content.
            char uri[256];
            snprintf(uri, sizeof(uri),
                "x-sonos-spotify:spotify%%3atrack%%3a%s?sid=%d&amp;flags=32&amp;sn=%d",
                TRACK_ID, curSid, curSn);

            // Build correct DIDL metadata
            // 00032020 = Sonos object ID prefix for audioItem.musicTrack
            // 00020000 = Sonos object ID prefix for parent track container
            char sidStr[16];
            snprintf(sidStr, sizeof(sidStr), "%d", curSid);

            // item id = "00030020" + encoded URI (matches node-sonos-http-api exactly)
            char itemId[128];
            snprintf(itemId, sizeof(itemId), "00030020spotify%%3atrack%%3a%s", TRACK_ID);

            String rawMeta =
                String("<DIDL-Lite xmlns:dc=\"http://purl.org/dc/elements/1.1/\" "
                       "xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\" "
                       "xmlns:r=\"urn:schemas-rinconnetworks-com:metadata-1-0/\" "
                       "xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\">"
                       "<item id=\"") + itemId + "\" restricted=\"true\">"
                "<upnp:class>object.item.audioItem.musicTrack</upnp:class>"
                "<desc id=\"cdudn\" nameSpace=\"urn:schemas-rinconnetworks-com:metadata-1-0/\">"
                "SA_RINCON" + sidStr + "_X_#Svc" + sidStr + "-0-Token"
                "</desc>"
                "</item>"
                "</DIDL-Lite>";

            // Encode for SOAP transport
            rawMeta.replace("&", "&amp;");
            rawMeta.replace("<", "&lt;");
            rawMeta.replace(">", "&gt;");
            rawMeta.replace("\"", "&quot;");

            Serial.printf("    sid=%-4d  sn=%d  URI: x-sonos-spotify:spotify%%3atrack%%3a%s?sid=%d&flags=0&sn=%d\n",
                curSid, curSn, TRACK_ID, curSid, curSn);

            // Send SetAVTransportURI
            static char soapArgs[2048];
            snprintf(soapArgs, sizeof(soapArgs),
                "<InstanceID>0</InstanceID>"
                "<CurrentURI>%s</CurrentURI>"
                "<CurrentURIMetaData>%s</CurrentURIMetaData>",
                uri, rawMeta.c_str());

            String setResp = sonos.sendSOAP("AVTransport", "SetAVTransportURI", soapArgs);

            bool uriAccepted = setResp.length() > 0
                && setResp.indexOf("Fault")     < 0
                && setResp.indexOf("UPnPError") < 0;

            if (!uriAccepted) {
                Serial.print("      SetAVTransportURI REJECTED");
                // Extract UPnP error code for diagnosis
                String errCode = sonos.extractXML(setResp, "errorCode");
                String errDesc = sonos.extractXML(setResp, "errorDescription");
                if (errCode.length() > 0) {
                    Serial.printf(" (UPnP errorCode=%s", errCode.c_str());
                    if (errCode == "501") Serial.print(": Action Failed — Spotify not authenticated?");
                    if (errCode == "714") Serial.print(": Illegal MIME-type — wrong URI format");
                    if (errCode == "716") Serial.print(": Not subscribed");
                    if (errDesc.length() > 0) Serial.printf(", %s", errDesc.c_str());
                    Serial.print(")");
                } else if (setResp.length() == 0) {
                    Serial.print(" (no response — device unreachable?)");
                }
                Serial.println();
                delay(300);
                continue;
            }

            Serial.println("      SetAVTransportURI ACCEPTED ✓ — sending Play...");
            delay(1000);  // brief pause: Sonos resolves Spotify URI asynchronously

            String playResp = sonos.sendSOAP("AVTransport", "Play",
                "<InstanceID>0</InstanceID><Speed>1</Speed>");

            bool playAccepted = playResp.length() > 0
                && playResp.indexOf("Fault")    < 0
                && playResp.indexOf("UPnPError") < 0;

            if (playAccepted) {
                Serial.printf("      Play ACCEPTED ✓  (sid=%d sn=%d)\n", curSid, curSn);

                // Poll up to 15s for Spotify SMAPI to resolve metadata & start playback
                Serial.println("      Polling for playback state (up to 15s)...");
                bool playing = false;
                for (int i = 0; i < 8; i++) {
                    delay(2000);
                    sonos.updateTrackInfo();
                    sonos.updatePlaybackState();
                    Serial.printf("      [%2ds] isPlaying=%s  track='%s'\n",
                        (i + 1) * 2,
                        dev->isPlaying ? "YES" : "NO",
                        dev->currentTrack.c_str());
                    if (dev->isPlaying || dev->currentTrack.length() > 0) {
                        playing = true;
                        break;
                    }
                }

                Serial.printf("\n      isPlaying : %s\n", dev->isPlaying ? "YES" : "NO");
                Serial.printf("      Track     : %s\n", dev->currentTrack.c_str());
                Serial.printf("      Artist    : %s\n", dev->currentArtist.c_str());
                Serial.printf("      Album     : %s\n", dev->currentAlbum.c_str());
                Serial.printf("      URI       : %s\n", dev->currentURI.c_str());

                if (playing) {
                    Serial.println("\n  SUCCESS! Track is playing.");
                } else {
                    Serial.println("\n  Play accepted but no state after 15s.");
                    Serial.println("  Possible causes:");
                    Serial.println("  - Spotify account not authenticated in Sonos app");
                    Serial.println("  - Wrong sn value for this account");
                    Serial.println("  - Play something from the Sonos app first, then retry");
                }

                Serial.println("=== END ===\n");
                return;
            } else {
                Serial.print("      Play REJECTED");
                String errCode = sonos.extractXML(playResp, "errorCode");
                if (errCode.length() > 0) {
                    Serial.printf(" (errorCode=%s)", errCode.c_str());
                }
                Serial.println();
            }

            delay(300);
        }
    }

    // ── All combinations failed ───────────────────────────────────────────────
    Serial.println("\n  All combinations failed. Troubleshooting:");
    Serial.println("  1. Type 'services' and look for Spotify + its Id value");
    Serial.println("  2. Make sure Spotify is linked in the Sonos app");
    Serial.println("  3. Play any Spotify track from the Sonos app to authorize");
    Serial.println("  4. Type 'uri' to see what the speaker currently has set");
    Serial.println("  5. Type 'transport' to check speaker transport state");
    Serial.println("=== END ===\n");
}


// Play a Spotify album via x-rincon-cpcontainer URI (SetAVTransportURI directly)
// Sonos loads the full album through SMAPI with complete metadata & album art
void SerialCommands::spotifyAlbumTest() {
    Serial.println("\n=== SPOTIFY ALBUM CONTAINER TEST ===");

    const char* ALBUM_ID   = "2dfTV7CktUEBkZCHiB7VQB";
    const char* ALBUM_NAME = "Test Album";

    int sid = sonos.getSpotifyServiceId();
    if (sid <= 0) { Serial.println("    sid not found — defaulting to 12"); sid = 12; }
    Serial.printf("[1] Spotify service ID: %d\n", sid);

    // Container URI — colons encoded as %3a (lowercase, as Sonos expects)
    char uri[256];
    snprintf(uri, sizeof(uri),
             "x-rincon-cpcontainer:0006206cspotify%%3aalbum%%3a%s", ALBUM_ID);
    Serial.printf("[2] URI: %s\n", uri);

    // DIDL: item id and parentID use the Sonos album object-ID prefix 0006206c
    // upnp:class must be object.container.album.musicAlbum for albums
    char sidStr[8];
    snprintf(sidStr, sizeof(sidStr), "%d", sid);
    String rawMeta =
        String("<DIDL-Lite xmlns:dc=\"http://purl.org/dc/elements/1.1/\" "
               "xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\" "
               "xmlns:r=\"urn:schemas-rinconnetworks-com:metadata-1-0/\" "
               "xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\">"
               "<item id=\"0006206cspotify%3aalbum%3a") + ALBUM_ID +
        "\" parentID=\"0002006cspotify%3aalbum%3a\" restricted=\"true\">"
        "<dc:title>" + ALBUM_NAME + "</dc:title>"
        "<upnp:class>object.container.album.musicAlbum</upnp:class>"
        "<desc id=\"cdudn\" nameSpace=\"urn:schemas-rinconnetworks-com:metadata-1-0/\">"
        "SA_RINCON" + sidStr + "_X_#Svc" + sidStr + "-5-Token"
        "</desc>"
        "</item>"
        "</DIDL-Lite>";

    // Encode DIDL for SOAP transport
    rawMeta.replace("&", "&amp;");
    rawMeta.replace("<", "&lt;");
    rawMeta.replace(">", "&gt;");
    rawMeta.replace("\"", "&quot;");

    // SetAVTransportURI with the container URI — Sonos loads the album into its queue
    // and fetches full metadata via Spotify SMAPI before playback starts
    static char soapArgs[2048];
    snprintf(soapArgs, sizeof(soapArgs),
        "<InstanceID>0</InstanceID>"
        "<CurrentURI>%s</CurrentURI>"
        "<CurrentURIMetaData>%s</CurrentURIMetaData>",
        uri, rawMeta.c_str());

    Serial.println("[3] Sending SetAVTransportURI...");
    String resp = sonos.sendSOAP("AVTransport", "SetAVTransportURI", soapArgs);

    bool accepted = resp.length() > 0 && resp.indexOf("Fault") < 0 && resp.indexOf("UPnPError") < 0;
    if (!accepted) {
        String errCode = sonos.extractXML(resp, "errorCode");
        Serial.printf("    REJECTED (errorCode=%s)\n", errCode.c_str());
        Serial.println("=== END ===\n");
        return;
    }
    Serial.println("    ACCEPTED ✓");

    delay(1500);
    Serial.println("[4] Sending Play...");
    String playResp = sonos.sendSOAP("AVTransport", "Play",
        "<InstanceID>0</InstanceID><Speed>1</Speed>");

    if (playResp.length() == 0 || playResp.indexOf("Fault") >= 0) {
        String errCode = sonos.extractXML(playResp, "errorCode");
        Serial.printf("    Play REJECTED (errorCode=%s)\n", errCode.c_str());
        Serial.println("=== END ===\n");
        return;
    }
    Serial.println("    Play ACCEPTED ✓");

    // Poll for state
    Serial.println("[5] Polling state (up to 15s)...");
    SonosDevice* dev = sonos.getCurrentDevice();
    bool got_meta = false;
    for (int i = 0; i < 8 && dev; i++) {
        delay(2000);
        sonos.updateTrackInfo();
        sonos.updatePlaybackState();
        Serial.printf("    [%2ds] isPlaying=%s  track='%s'\n",
            (i + 1) * 2, dev->isPlaying ? "YES" : "NO", dev->currentTrack.c_str());
        if (dev->isPlaying || dev->currentTrack.length() > 0) { got_meta = true; break; }
    }

    if (dev) {
        Serial.printf("\n    isPlaying : %s\n", dev->isPlaying ? "YES" : "NO");
        Serial.printf("    Track     : %s\n", dev->currentTrack.c_str());
        Serial.printf("    Artist    : %s\n", dev->currentArtist.c_str());
        Serial.printf("    Album     : %s\n", dev->currentAlbum.c_str());
        Serial.printf("    ArtURL    : %s\n", dev->albumArtURL.c_str());
    }
    Serial.println(got_meta ? "\n  SUCCESS!" : "\n  Timed out waiting for state.");
    Serial.println("=== END ===\n");
}


// Alternative method using SetAVTransportURI directly
void SerialCommands::playSpotifyAlbumViaSetAVTransportURI(const String& uri, const String& metadata, const String& name) {
    Serial.println("\n[ALT] Trying SetAVTransportURI instead...");
    
    String soapBody = String("<InstanceID>0</InstanceID>") +
                     "<CurrentURI>" + uri + "</CurrentURI>" +
                     "<CurrentURIMetaData>" + metadata + "</CurrentURIMetaData>";
    
    String response = sonos.sendSOAP("AVTransport", "SetAVTransportURI", soapBody.c_str());
    
    if (response.length() > 0 && response.indexOf("Fault") < 0) {
        Serial.println("✓ URI set successfully");
        
        delay(1000);
        
        // Play
        String playResp = sonos.sendSOAP("AVTransport", "Play", 
            "<InstanceID>0</InstanceID><Speed>1</Speed>");
            
        if (playResp.length() > 0 && playResp.indexOf("Fault") < 0) {
            Serial.println("✓ Playing...");
            delay(3000);
            getCurrentInfo();
        } else {
            Serial.println("✗ Play command failed");
        }
    } else {
        Serial.println("✗ SetAVTransportURI failed");
    }
}

// Add a comprehensive album play test
void SerialCommands::testAlbumFromCapture() {
    Serial.println("\n=== ALBUM PLAY TEST FROM CAPTURE ===");
    
    // Test multiple albums from your test data
    for (int i = 0; i < NUM_TEST_ALBUMS; i++) {
        Serial.printf("\n--- Testing Album %d: %s ---\n", i+1, TEST_ALBUMS[i].name);
        Serial.printf("URI: %s\n", TEST_ALBUMS[i].uri);
        
        // Extract just the ID part
        String albumId = TEST_ALBUMS[i].uri;
        if (albumId.startsWith("spotify:album:")) {
            albumId = albumId.substring(14);
        }
        
        // Build the URI
        char encodedUri[256];
        snprintf(encodedUri, sizeof(encodedUri), 
                 "x-rincon-cpcontainer:0006206cspotify%%3Aalbum%%3A%s", albumId.c_str());
        
        // Build metadata
        String metadata = "<DIDL-Lite xmlns:dc=\"http://purl.org/dc/elements/1.1/\" ";
        metadata += "xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\" ";
        metadata += "xmlns:r=\"urn:schemas-rinconnetworks-com:metadata-1-0/\" ";
        metadata += "xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\">";
        metadata += "<item id=\"00030020spotify:album:";
        metadata += albumId;
        metadata += "\" parentID=\"0002006cspotify:album:";
        metadata += albumId;
        metadata += "\" restricted=\"true\">";
        metadata += "<dc:title>";
        metadata += TEST_ALBUMS[i].name;
        metadata += "</dc:title>";
        metadata += "<upnp:class>object.container.album.musicAlbum</upnp:class>";
        
        // Get Spotify service ID
        int sid = sonos.getSpotifyServiceId();
        if (sid <= 0) sid = 12;
        
        metadata += "<desc id=\"cdudn\" nameSpace=\"urn:schemas-rinconnetworks-com:metadata-1-0/\">";
        metadata += "SA_RINCON";
        metadata += String(sid);
        metadata += "_X_#Svc";
        metadata += String(sid);
        metadata += "-5-Token</desc>";
        metadata += "</item></DIDL-Lite>";
        
        // Encode for SOAP
        metadata.replace("&", "&amp;");
        metadata.replace("<", "&lt;");
        metadata.replace(">", "&gt;");
        metadata.replace("\"", "&quot;");
        
        // Send AddURIToQueue
        String soapBody = String("<InstanceID>0</InstanceID>") +
                         "<EnqueuedURI>" + encodedUri + "</EnqueuedURI>" +
                         "<EnqueuedURIMetaData>" + metadata + "</EnqueuedURIMetaData>" +
                         "<DesiredFirstTrackNumberEnqueued>1</DesiredFirstTrackNumberEnqueued>" +
                         "<EnqueueAsNext>1</EnqueueAsNext>";
        
        String response = sonos.sendSOAP("AVTransport", "AddURIToQueue", soapBody.c_str());
        
        if (response.length() > 0 && response.indexOf("Fault") < 0) {
            Serial.println("✓ Album queued successfully");
            
            String firstTrack = sonos.extractXML(response, "FirstTrackNumberEnqueued");
            String numTracks = sonos.extractXML(response, "NumTracksAdded");
            
            if (firstTrack.length() > 0 && numTracks.length() > 0) {
                Serial.printf("  Tracks %s-%s added to queue\n", 
                    firstTrack.c_str(),
                    (String(firstTrack.toInt() + numTracks.toInt() - 1)).c_str());
            }
            
            delay(1000);
            
        } else {
            Serial.println("✗ Failed to queue album");
            
            // Try direct play as fallback
            Serial.println("Trying direct playback...");
            soapBody = String("<InstanceID>0</InstanceID>") +
                      "<CurrentURI>" + encodedUri + "</CurrentURI>" +
                      "<CurrentURIMetaData>" + metadata + "</CurrentURIMetaData>";
            
            response = sonos.sendSOAP("AVTransport", "SetAVTransportURI", soapBody.c_str());
            
            if (response.length() > 0 && response.indexOf("Fault") < 0) {
                Serial.println("✓ URI set, playing...");
                delay(1000);
                sonos.sendSOAP("AVTransport", "Play", "<InstanceID>0</InstanceID><Speed>1</Speed>");
                delay(3000);
                getCurrentInfo();
            }
        }
        
        delay(2000);  // Wait between tests
    }
    
    Serial.println("\n=== ALL TESTS COMPLETE ===");
    Serial.println("Type 'info' to see current playback status");
    Serial.println("=== END ===\n");
}
// Queue-based Spotify track playback:
//   AddURIToQueue → x-rincon-queue:{RINCON}#0 → Seek → Play
// Advantage over direct x-sonos-spotify: Sonos stores the DIDL locally so
// GetPositionInfo returns real metadata immediately on the first poll.
void SerialCommands::queueTest() {
    Serial.println("\n=== SPOTIFY QUEUE TRACK TEST ===");

    const char* TRACK_ID   = "3iHzKA9HlXf5wsGdsrsnSA";
    const char* TRACK_NAME = "Test Track";

    SonosDevice* dev = sonos.getCurrentDevice();
    if (!dev) {
        Serial.println("ERROR: No device selected.");
        Serial.println("=== END ===\n");
        return;
    }
    Serial.printf("Device : %s  IP: %s\n", dev->roomName.c_str(), dev->ip.toString().c_str());
    Serial.printf("RINCON : %s\n", dev->rinconID.c_str());

    if (dev->rinconID.length() == 0) {
        Serial.println("ERROR: RINCON ID not populated — run discovery first.");
        Serial.println("=== END ===\n");
        return;
    }

    // Step 1: Detect group coordinator + check if already in queue mode.
    // Done ONCE here, before the sn loop — GetMediaInfo becomes unreliable after
    // RemoveAllTracksFromQueue and may return 402 mid-loop.
    String coordinatorRincon = dev->rinconID;  // default: assume self is coordinator
    char queueURI[128];
    bool alreadyInQueueMode = false;
    {
        String mediaResp = sonos.sendSOAP("AVTransport", "GetMediaInfo", "<InstanceID>0</InstanceID>");
        String currentURI = sonos.extractXML(mediaResp, "CurrentURI");
        Serial.printf("[1] GetMediaInfo CurrentURI: %s\n", currentURI.c_str());
        if (currentURI.startsWith("x-rincon:")) {
            coordinatorRincon = currentURI.substring(9);  // strip "x-rincon:"
            Serial.printf("    Device is a GROUP SLAVE. Coordinator RINCON: %s\n", coordinatorRincon.c_str());
        } else {
            Serial.println("    Device is STANDALONE / coordinator. Using own RINCON.");
        }
        snprintf(queueURI, sizeof(queueURI), "x-rincon-queue:%s#0", coordinatorRincon.c_str());
        alreadyInQueueMode = (currentURI == String(queueURI));
        if (alreadyInQueueMode) Serial.println("    Transport already in queue mode.");
    }

    // Step 2: Discover sid/sn
    int sid = sonos.getSpotifyServiceId();
    if (sid <= 0) { Serial.println("  sid not found — defaulting to 12"); sid = 12; }
    Serial.printf("[2] sid=%d\n", sid);

    char sidStr[8];
    snprintf(sidStr, sizeof(sidStr), "%d", sid);

    // sn=5 confirmed working (observed from native Sonos app). flags=0 confirmed correct.
    int snCandidates[] = {5, 2, 1, 3, 4, 0};

    for (int i = 0; snCandidates[i] != 0; i++) {
        int sn = snCandidates[i];
        Serial.printf("\n[3] Trying sn=%d...\n", sn);

        // Step 3: Clear queue
        sonos.sendSOAP("AVTransport", "RemoveAllTracksFromQueue", "<InstanceID>0</InstanceID>");
        delay(300);

        // Step 3: Build AddURIToQueue DIDL matching node-sonos-http-api exactly.
        // Key differences from our previous attempts:
        //   flags=32 (node-sonos-http-api uses this, not 0 or 8224)
        //   item id = "00030020" + encoded spotify URI (no parentID needed)
        //   desc token: SA_RINCON{sid}_X_#Svc{sid}-0-Token (-0- is always literal 0, not sn)
        //   No <res> element — SMAPI resolves metadata via the desc token
        char enqueuedURI[256];
        snprintf(enqueuedURI, sizeof(enqueuedURI),
            "x-sonos-spotify:spotify%%3atrack%%3a%s?sid=%d&amp;flags=32&amp;sn=%d",
            TRACK_ID, sid, sn);

        char itemId[128];
        snprintf(itemId, sizeof(itemId), "00030020spotify%%3atrack%%3a%s", TRACK_ID);

        String rawMeta =
            String("<DIDL-Lite xmlns:dc=\"http://purl.org/dc/elements/1.1/\" "
                   "xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\" "
                   "xmlns:r=\"urn:schemas-rinconnetworks-com:metadata-1-0/\" "
                   "xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\">"
                   "<item id=\"") + itemId + "\" restricted=\"true\">"
            "<upnp:class>object.item.audioItem.musicTrack</upnp:class>"
            "<desc id=\"cdudn\" nameSpace=\"urn:schemas-rinconnetworks-com:metadata-1-0/\">"
            "SA_RINCON" + sidStr + "_X_#Svc" + sidStr + "-0-Token"
            "</desc>"
            "</item>"
            "</DIDL-Lite>";

        // Encode DIDL for SOAP transport
        rawMeta.replace("&", "&amp;");
        rawMeta.replace("<", "&lt;");
        rawMeta.replace(">", "&gt;");
        rawMeta.replace("\"", "&quot;");

        static char addArgs[2048];
        snprintf(addArgs, sizeof(addArgs),
            "<InstanceID>0</InstanceID>"
            "<EnqueuedURI>%s</EnqueuedURI>"
            "<EnqueuedURIMetaData>%s</EnqueuedURIMetaData>"
            "<DesiredFirstTrackNumberEnqueued>1</DesiredFirstTrackNumberEnqueued>"
            "<EnqueueAsNext>0</EnqueueAsNext>",
            enqueuedURI, rawMeta.c_str());

        String addResp = sonos.sendSOAP("AVTransport", "AddURIToQueue", addArgs);
        bool added = addResp.length() > 0 && addResp.indexOf("Fault") < 0 && addResp.indexOf("UPnPError") < 0;
        if (!added) {
            String err = sonos.extractXML(addResp, "errorCode");
            Serial.printf("    AddURIToQueue REJECTED (errorCode=%s)\n", err.c_str());
            delay(300);
            continue;
        }
        // Extract actual queue position to seek to — the queue may not be empty
        String firstTrackStr = sonos.extractXML(addResp, "FirstTrackNumberEnqueued");
        int seekTrack = firstTrackStr.length() > 0 ? firstTrackStr.toInt() : 1;
        Serial.printf("    AddURIToQueue ACCEPTED ✓  (queued at position %d)\n", seekTrack);

        // Step 4: Point transport at queue — skip if already in queue mode.
        // We use the alreadyInQueueMode flag determined ONCE before the sn loop.
        // Re-checking inside the loop is unreliable: RemoveAllTracksFromQueue + AddURIToQueue
        // can temporarily leave the device in a state where GetMediaInfo returns 402.
        if (alreadyInQueueMode) {
            Serial.println("    Transport already in queue mode — skipping SetAVTransportURI");
        } else {
            static char setArgs[256];
            snprintf(setArgs, sizeof(setArgs),
                "<InstanceID>0</InstanceID>"
                "<CurrentURI>%s</CurrentURI>"
                "<CurrentURIMetaData></CurrentURIMetaData>",
                queueURI);

            String setResp = sonos.sendSOAP("AVTransport", "SetAVTransportURI", setArgs);
            if (setResp.length() == 0 || setResp.indexOf("Fault") >= 0) {
                String err = sonos.extractXML(setResp, "errorCode");
                Serial.printf("    SetAVTransportURI REJECTED (errorCode=%s) — trying Seek+Play anyway\n", err.c_str());
                // Don't continue — fall through to Seek+Play. After AddURIToQueue
                // some devices enter queue mode implicitly and will accept Play.
            } else {
                Serial.printf("    SetAVTransportURI (queue) ACCEPTED ✓\n");
            }
        }

        // Step 5: Seek to the track's queue position
        delay(200);
        {
            static char seekArgs[128];
            snprintf(seekArgs, sizeof(seekArgs),
                "<InstanceID>0</InstanceID><Unit>TRACK_NR</Unit><Target>%d</Target>", seekTrack);
            String seekResp = sonos.sendSOAP("AVTransport", "Seek", seekArgs);
            Serial.printf("    Seek to track %d: %s\n", seekTrack,
                (seekResp.length() > 0 && seekResp.indexOf("Fault") < 0) ? "ACCEPTED ✓" : "REJECTED");
        }

        // Step 6: Play
        delay(200);
        String playResp = sonos.sendSOAP("AVTransport", "Play",
            "<InstanceID>0</InstanceID><Speed>1</Speed>");
        bool playOK = playResp.length() > 0 && playResp.indexOf("Fault") < 0;
        if (!playOK) {
            Serial.printf("    Play REJECTED\n");
            delay(300);
            continue;
        }
        Serial.printf("    Play ACCEPTED ✓  (sid=%d sn=%d)\n", sid, sn);

        // Step 7: Poll for state
        Serial.println("    Polling for state (up to 15s)...");
        bool got_meta = false;
        for (int p = 0; p < 8; p++) {
            delay(2000);
            sonos.updateTrackInfo();
            sonos.updatePlaybackState();
            Serial.printf("    [%2ds] isPlaying=%s  track='%s'\n",
                (p + 1) * 2, dev->isPlaying ? "YES" : "NO", dev->currentTrack.c_str());
            if (dev->isPlaying || dev->currentTrack.length() > 0) { got_meta = true; break; }
        }

        Serial.printf("\n    isPlaying : %s\n", dev->isPlaying ? "YES" : "NO");
        Serial.printf("    Track     : %s\n", dev->currentTrack.c_str());
        Serial.printf("    Artist    : %s\n", dev->currentArtist.c_str());
        Serial.printf("    Album     : %s\n", dev->currentAlbum.c_str());
        Serial.printf("    ArtURL    : %s\n", dev->albumArtURL.c_str());
        Serial.printf("    URI       : %s\n", dev->currentURI.c_str());

        Serial.println(got_meta ? "\n  SUCCESS!" : "\n  Timed out waiting for state.");
        Serial.println("=== END ===\n");
        return;
    }

    Serial.println("\n  All sn values rejected.");
    Serial.println("=== END ===\n");
}
