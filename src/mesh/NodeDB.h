#pragma once

#include "Observer.h"
#include <Arduino.h>
#include <algorithm>
#include <assert.h>
#include <pb_encode.h>
#include <vector>

#include "MeshTypes.h"
#include "NodeStatus.h"
#include "configuration.h"
#include "mesh-pb-constants.h"
#include "mesh/generated/meshtastic/mesh.pb.h" // For CriticalErrorCode

#if ARCH_PORTDUINO
#include "PortduinoGlue.h"
#endif

/*
DeviceState versions used to be defined in the .proto file but really only this function cares.  So changed to a
#define here.
*/

#define SEGMENT_CONFIG 1
#define SEGMENT_MODULECONFIG 2
#define SEGMENT_DEVICESTATE 4
#define SEGMENT_CHANNELS 8
#define SEGMENT_NODEDATABASE 16

#define DEVICESTATE_CUR_VER 24
#define DEVICESTATE_MIN_VER 24

extern meshtastic_DeviceState devicestate;
extern meshtastic_NodeDatabase nodeDatabase;
extern meshtastic_ChannelFile channelFile;
extern meshtastic_MyNodeInfo &myNodeInfo;
extern meshtastic_LocalConfig config;
extern meshtastic_DeviceUIConfig uiconfig;
extern meshtastic_LocalModuleConfig moduleConfig;
extern meshtastic_User &owner;
extern meshtastic_Position localPosition;

static constexpr const char *deviceStateFileName = "/prefs/device.proto";
static constexpr const char *legacyPrefFileName = "/prefs/db.proto";
static constexpr const char *nodeDatabaseFileName = "/prefs/nodes.proto";
static constexpr const char *configFileName = "/prefs/config.proto";
static constexpr const char *uiconfigFileName = "/prefs/uiconfig.proto";
static constexpr const char *moduleConfigFileName = "/prefs/module.proto";
static constexpr const char *channelFileName = "/prefs/channels.proto";
static constexpr const char *backupFileName = "/backups/backup.proto";

/// Given a node, return how many seconds in the past (vs now) that we last heard from it
uint32_t sinceLastSeen(const meshtastic_NodeInfoLite *n);

/// Given a packet, return how many seconds in the past (vs now) it was received
uint32_t sinceReceived(const meshtastic_MeshPacket *p);

enum LoadFileResult {
    // Successfully opened the file
    LOAD_SUCCESS = 1,
    // File does not exist
    NOT_FOUND = 2,
    // Device does not have a filesystem
    NO_FILESYSTEM = 3,
    // File exists, but could not decode protobufs
    DECODE_FAILED = 4,
    // File exists, but open failed for some reason
    OTHER_FAILURE = 5
};

enum UserLicenseStatus { NotKnown, NotLicensed, Licensed };

class NodeDB
{
    // NodeNum provisionalNodeNum; // if we are trying to find a node num this is our current attempt

    // A NodeInfo for every node we've seen
    // Eventually use a smarter datastructure
    // HashMap<NodeNum, NodeInfo> nodes;
    // Note: these two references just point into our static array we serialize to/from disk

  public:
    std::vector<meshtastic_NodeInfoLite> *meshNodes;
    bool updateGUI = false; // we think the gui should definitely be redrawn, screen will clear this once handled
    meshtastic_NodeInfoLite *updateGUIforNode = NULL; // if currently showing this node, we think you should update the GUI
    Observable<const meshtastic::NodeStatus *> newStatus;
    pb_size_t numMeshNodes;

    /// don't do mesh based algorithm for node id assignment (initially)
    /// instead just store in flash - possibly even in the initial alpha release do this hack
    NodeDB();

    /// write to flash
    /// @return true if the save was successful
    bool saveToDisk(int saveWhat = SEGMENT_CONFIG | SEGMENT_MODULECONFIG | SEGMENT_DEVICESTATE | SEGMENT_CHANNELS |
                                   SEGMENT_NODEDATABASE);

    /** Reinit radio config if needed, because either:
     * a) sometimes a buggy android app might send us bogus settings or
     * b) the client set factory_reset
     *
     * @param factory_reset if true, reset all settings to factory defaults
     * @param is_fresh_install set to true after a fresh install, to trigger NodeInfo/Position requests
     * @return true if the config was completely reset, in that case, we should send it back to the client
     */
    void resetRadioConfig(bool is_fresh_install = false);

    /// given a subpacket sniffed from the network, update our DB state
    /// we updateGUI and updateGUIforNode if we think our this change is big enough for a redraw
    void updateFrom(const meshtastic_MeshPacket &p);

    void addFromContact(const meshtastic_SharedContact);

    /** Update position info for this node based on received position data
     */
    void updatePosition(uint32_t nodeId, const meshtastic_Position &p, RxSource src = RX_SRC_RADIO);

    /** Update telemetry info for this node based on received metrics
     */
    void updateTelemetry(uint32_t nodeId, const meshtastic_Telemetry &t, RxSource src = RX_SRC_RADIO);

    /** Update user info and channel for this node based on received user data
     */
    bool updateUser(uint32_t nodeId, meshtastic_User &p, uint8_t channelIndex = 0);

    /// @return our node number
    NodeNum getNodeNum() { return myNodeInfo.my_node_num; }

    // @return last byte of a NodeNum, 0xFF if it ended at 0x00
    uint8_t getLastByteOfNodeNum(NodeNum num) { return (uint8_t)((num & 0xFF) ? (num & 0xFF) : 0xFF); }

    /// if returns false, that means our node should send a DenyNodeNum response.  If true, we think the number is okay for use
    // bool handleWantNodeNum(NodeNum n);

    /* void handleDenyNodeNum(NodeNum FIXME read mesh proto docs, perhaps picking a random node num is not a great idea
    and instead we should use a special 'im unconfigured node number' and include our desired node number in the wantnum message.
    the unconfigured node num would only be used while initially joining the mesh so low odds of conflicting (especially if we
    randomly select from a small number of nodenums which can be used temporarily for this operation).  figure out what the lower
    level mesh sw does if it does conflict?  would it be better for people who are replying with denynode num to just broadcast
    their denial?)
    */

    /// pick a provisional nodenum we hope no one is using
    void pickNewNodeNum();

    // get channel channel index we heard a nodeNum on, defaults to 0 if not found
    uint8_t getMeshNodeChannel(NodeNum n);

    /* Return the number of nodes we've heard from recently (within the last 2 hrs?)
     * @param localOnly if true, ignore nodes heard via MQTT
     */
    size_t getNumOnlineMeshNodes(bool localOnly = false);

    void initConfigIntervals(), initModuleConfigIntervals(), resetNodes(), removeNodeByNum(NodeNum nodeNum);

    bool factoryReset(bool eraseBleBonds = false);

    LoadFileResult loadProto(const char *filename, size_t protoSize, size_t objSize, const pb_msgdesc_t *fields,
                             void *dest_struct);
    bool saveProto(const char *filename, size_t protoSize, const pb_msgdesc_t *fields, const void *dest_struct,
                   bool fullAtomic = true);

    void installRoleDefaults(meshtastic_Config_DeviceConfig_Role role);

    const meshtastic_NodeInfoLite *readNextMeshNode(uint32_t &readIndex);

    meshtastic_NodeInfoLite *getMeshNodeByIndex(size_t x)
    {
        assert(x < numMeshNodes);
        return &meshNodes->at(x);
    }

    virtual meshtastic_NodeInfoLite *getMeshNode(NodeNum n);
    size_t getNumMeshNodes() { return numMeshNodes; }

    UserLicenseStatus getLicenseStatus(uint32_t nodeNum);

    size_t getMaxNodesAllocatedSize()
    {
        meshtastic_NodeDatabase emptyNodeDatabase;
        emptyNodeDatabase.version = DEVICESTATE_CUR_VER;
        size_t nodeDatabaseSize;
        pb_get_encoded_size(&nodeDatabaseSize, meshtastic_NodeDatabase_fields, &emptyNodeDatabase);
        return nodeDatabaseSize + (MAX_NUM_NODES * meshtastic_NodeInfoLite_size);
    }

    // returns true if the maximum number of nodes is reached or we are running low on memory
    bool isFull();

    void clearLocalPosition();

    void setLocalPosition(meshtastic_Position position, bool timeOnly = false)
    {
        if (timeOnly) {
            LOG_DEBUG("Set local position time only: time=%u timestamp=%u", position.time, position.timestamp);
            localPosition.time = position.time;
            localPosition.timestamp = position.timestamp > 0 ? position.timestamp : position.time;
            return;
        }
        LOG_DEBUG("Set local position: lat=%i lon=%i time=%u timestamp=%u", position.latitude_i, position.longitude_i,
                  position.time, position.timestamp);
        localPosition = position;
    }

    bool hasValidPosition(const meshtastic_NodeInfoLite *n);

    bool backupPreferences(meshtastic_AdminMessage_BackupLocation location);
    bool restorePreferences(meshtastic_AdminMessage_BackupLocation location,
                            int restoreWhat = SEGMENT_CONFIG | SEGMENT_MODULECONFIG | SEGMENT_DEVICESTATE | SEGMENT_CHANNELS);

  private:
    uint32_t lastNodeDbSave = 0;    // when we last saved our db to flash
    uint32_t lastBackupAttempt = 0; // when we last tried a backup automatically or manually
    /// Find a node in our DB, create an empty NodeInfoLite if missing
    meshtastic_NodeInfoLite *getOrCreateMeshNode(NodeNum n);

    /// Notify observers of changes to the DB
    void notifyObservers(bool forceUpdate = false)
    {
        // Notify observers of the current node state
        const meshtastic::NodeStatus status = meshtastic::NodeStatus(getNumOnlineMeshNodes(), getNumMeshNodes(), forceUpdate);
        newStatus.notifyObservers(&status);
    }

    /// read our db from flash
    void loadFromDisk();

    /// purge db entries without user info
    void cleanupMeshDB();

    /// Reinit device state from scratch (not loading from disk)
    void installDefaultDeviceState(), installDefaultNodeDatabase(), installDefaultChannels(),
        installDefaultConfig(bool preserveKey), installDefaultModuleConfig();

    /// write to flash
    /// @return true if the save was successful
    bool saveToDiskNoRetry(int saveWhat);

    bool saveChannelsToDisk();
    bool saveDeviceStateToDisk();
    bool saveNodeDatabaseToDisk();
};

extern NodeDB *nodeDB;

/*
  If is_router is set, we use a number of different default values

        # FIXME - after tuning, move these params into the on-device defaults based on is_router and is_power_saving

        # prefs.position_broadcast_secs = FIXME possibly broadcast only once an hr
        prefs.wait_bluetooth_secs = 1  # Don't stay in bluetooth mode
        # try to stay in light sleep one full day, then briefly wake and sleep again

        prefs.ls_secs = oneday

        prefs.position_broadcast_secs = 12 hours # send either position or owner every 12hrs

        # get a new GPS position once per day
        prefs.gps_update_interval = oneday

        prefs.is_power_saving = True
*/

/** The current change # for radio settings.  Starts at 0 on boot and any time the radio settings
 * might have changed is incremented.  Allows others to detect they might now be on a new channel.
 */
extern uint32_t radioGeneration;

extern meshtastic_CriticalErrorCode error_code;

/*
 * A numeric error address (nonzero if available)
 */
extern uint32_t error_address;
#define NODEINFO_BITFIELD_IS_KEY_MANUALLY_VERIFIED_SHIFT 0
#define NODEINFO_BITFIELD_IS_KEY_MANUALLY_VERIFIED_MASK (1 << NODEINFO_BITFIELD_IS_KEY_MANUALLY_VERIFIED_SHIFT)

#define Module_Config_size                                                                                                       \
    (ModuleConfig_CannedMessageConfig_size + ModuleConfig_ExternalNotificationConfig_size + ModuleConfig_MQTTConfig_size +       \
     ModuleConfig_RangeTestConfig_size + ModuleConfig_SerialConfig_size + ModuleConfig_StoreForwardConfig_size +                 \
     ModuleConfig_TelemetryConfig_size + ModuleConfig_size)

// Please do not remove this comment, it makes trunk and compiler happy at the same time.