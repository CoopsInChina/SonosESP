#ifndef SERIALCOMMANDS_H
#define SERIALCOMMANDS_H

#include <Arduino.h>
#include "sonos_controller.h"

class SerialCommands {
public:
    SerialCommands(SonosController& sonosRef);
    void begin();
    void handleInput();
    void processCommand(const String& command);  // Public: called from main loop
    
    // Debug/test commands
    void help();
    void testConnection();
    void playTestRadio();
    void playTestSpotify();
    void spotifyTest();  // Spotify test with auto service ID discovery
    void playTestLocalFile();
    void playSpotifyTrack(const String& uri, const String& name);
    void playSpotifyAlbum(const String& uri, const String& name);
    void playSpotifyPlaylist(const String& uri, const String& name);
    void clearQueue();
    void listServices();
    void getCurrentInfo();
    void getCurrentURI();
    void rawpos();  // Dump raw GetPositionInfo XML
    void getTransportInfo();
    void getMediaInfo();
    void debugMemory();
    void testQueueLoad();
    void playQueueItem(int index);
    void getGroupInfo();
    void joinGroups();
    void leaveAllGroups();
    void testAlbumFromCapture();
    void playSpotifyAlbumViaSetAVTransportURI(const String& uri, const String& metadata, const String& name);    
    void spotifyAlbumTest();
    void queueTest();         // Queue-based single track playback (fixes metadata)
    
private:
    SonosController& sonos;
    String inputBuffer;

    void processCommandWithArgs(const String& cmd, const String& args);
    void processPlayCommand(const String& args);
    void processGroupCommand(const String& args);
    
    // Test tracks/albums/playlists
    struct TestContent {
        const char* uri;
        const char* name;
    };
    
    static const TestContent TEST_TRACKS[];
    static const TestContent TEST_ALBUMS[];
    static const TestContent TEST_PLAYLISTS[];
    static const int NUM_TEST_TRACKS = 5;
    static const int NUM_TEST_ALBUMS = 3;
    static const int NUM_TEST_PLAYLISTS = 3;
};

#endif // SERIALCOMMANDS_H