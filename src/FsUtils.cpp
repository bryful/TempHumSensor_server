#include "FsUtils.hpp"

FsUtils::FsUtils()
{
    // prrference.begin(PREF_NAMESPACE, false);
}
FsUtils::~FsUtils()
{
    // pref.end();
}

int FsUtils::split(String data, char delimiter, String *dst)
{
    int index = 0;
    int datalength = data.length();

    for (int i = 0; i < datalength; i++)
    {
        char tmp = data.charAt(i);
        if (tmp == delimiter)
        {
            index++;
        }
        else
            dst[index] += tmp;
    }

    return (index + 1);
}
// -----------------------------------------------------------------------------
bool FsUtils::prefClear()
{
    bool ret = false;
    Preferences pref;
    if (pref.begin(PREF_NAMESPACE, false) == false)
        return ret;
    ret = pref.clear();
    pref.end();
    return ret;
}

// -----------------------------------------------------------------------------
String FsUtils::getPrefString(String key, String def)
{
    String ret = def;
    Preferences pref;
    if (pref.begin(PREF_NAMESPACE, false) == false)
        return ret;
    ret = pref.getString(key.c_str(), def);

    pref.end();
    return ret;
}
bool FsUtils::getPrefIPA(String key, int *adr)
{
    Serial.println("getPrefIPA");
    bool ret = false;
    String s = getPrefString(key, "");
    Serial.println("ppp :" + s);
    if (s != "")
    {
        String aaa[4];
        int idx = split(s, '.', aaa);
        if (idx >= 4)
        {
            adr[0] = atoi(aaa[0].c_str()) & 0xFF;
            adr[1] = atoi(aaa[1].c_str()) & 0xFF;
            adr[2] = atoi(aaa[2].c_str()) & 0xFF;
            adr[3] = atoi(aaa[3].c_str()) & 0xFF;
            ret = true;
        }
    }
    return ret;
}
// -----------------------------------------------------------------------------
String FsUtils::getBoardName(String def)
{
    String ret = def;
    ret = getPrefString(BOARD_KEY, def);

    return ret;
}
// -----------------------------------------------------------------------------
bool FsUtils::setPrefString(String key, String str)
{
    bool ret = false;
    Preferences pref;

    if (pref.begin(PREF_NAMESPACE, false) == false)
        return ret;
    ret = pref.putString(key.c_str(), str);
    delay(100);
    Serial.printf("putString result %d", ret);
    pref.end();
    return ret;
}
// -----------------------------------------------------------------------------
bool FsUtils::setBoardName(String bn)
{
    bool ret = false;

    ret = setPrefString(BOARD_KEY, bn);
    return ret;
}
// -----------------------------------------------------------------------------
String FsUtils::getDefaultMacAddress()
{

    String mac = "";

    unsigned char mac_base[6] = {0};

    if (esp_efuse_mac_get_default(mac_base) == ESP_OK)
    {
        char buffer[80]; // 6*2 characters for hex + 5 characters for colons + 1 character for null terminator
        sprintf(buffer, "%02X:%02X:%02X:%02X:%02X:%02X", mac_base[0], mac_base[1], mac_base[2], mac_base[3], mac_base[4], mac_base[5]);
        // sprintf(buffer, "%d:%d:%d:%d:%d:%d", mac_base[0], mac_base[1], mac_base[2], mac_base[3], mac_base[4], mac_base[5]);
        mac = buffer;
    }

    return mac;
}
// -----------------------------------------------------------------------------
String FsUtils::EPS_Status_json()
{
    JsonDocument doc;
    uint8_t mac0[6];
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    doc["MAC"] = getDefaultMacAddress();
    doc["cores"] = chip_info.cores;
    doc["revision"] = chip_info.revision;
    doc["flashsize"] = spi_flash_get_chip_size();
    doc["ESP-IDF"] = esp_get_idf_version();
    doc["ChipModel"] = ESP.getChipModel();
    doc["ChipRevision"] = ESP.getChipRevision();
    doc["FreeHeap"] = ESP.getFreeHeap();
    doc["HeapSize"] = ESP.getHeapSize();
    doc["PsramSize"] = ESP.getPsramSize();
    doc["FreePsram"] = ESP.getFreePsram();

    String ret;
    serializeJson(doc, ret);
    return ret;
}
String FsUtils::GetSerialReadAll()
{
    String ret = "";
    std::vector<byte> buff;
    if (Serial.available() <= 0)
        return ret;
    int err = 0;
    int cnt = 0;
    while (cnt < 2048)
    {
        if (Serial.available())
        {
            int idx = Serial.read();
            if (idx >= 0)
            {
                // buf[cnt] = idx;
                buff.push_back(idx);
                cnt++;
            }
        }
        else
        {
            break;
        }
    }
    if (cnt > 0)
    {
        // buf[cnt] = 0;
        buff.push_back((byte)0);
        ret = String((char *)buff.data());
    }
    return ret;
}
JsonDocument FsUtils::GetJsonData(String tx, bool *ok)
{
    JsonDocument ret;
    *ok = true;
    //  String tx = fsu.GetSerialReadAll();
    if (tx.length() > 0)
    {
        int idx0 = tx.indexOf("{");
        int idx1 = tx.lastIndexOf("}");
        if ((idx0 >= 0) && (idx0 < idx1))
        {
            String tx2 = tx.substring(idx0, idx1 - idx0 + 1);
            DeserializationError error = deserializeJson(ret, tx2);
            *ok = (error == DeserializationError::Ok);
        }
        else
        {
            *ok = false;
        }
    }
    return ret;
}
bool FsUtils::JsonCMDCheck(JsonDocument t, JsonDocument *r)
{
    bool ret = false;
    uint8_t mac0[6];
    esp_chip_info_t chip_info;
    esp_chip_info(&chip_info);

    ret = true;
    if (!t["getStatus"].isNull())
    {
        (*r)["MAC"] = getDefaultMacAddress();
        (*r)["cores"] = chip_info.cores;
        (*r)["revision"] = chip_info.revision;
        (*r)["flashsize"] = spi_flash_get_chip_size();
        (*r)["ESP-IDF"] = esp_get_idf_version();
        (*r)["ChipModel"] = ESP.getChipModel();
        (*r)["ChipRevision"] = ESP.getChipRevision();
        (*r)["FreeHeap"] = ESP.getFreeHeap();
        (*r)["HeapSize"] = ESP.getHeapSize();
        (*r)["PsramSize"] = ESP.getPsramSize();
        (*r)["FreePsram"] = ESP.getFreePsram();
    }
    if (!t["getBoardName"].isNull())
    {
        (*r)["BoardName"] = getBoardName("");
    }
    if (!t["setBoardName"].isNull())
    {
        String ss = t["setBoardName"].as<String>();
        if (setBoardName(ss) == false)
        {
            Serial.println("setBoardName errr : " + ss);
            ret = false;
        }
        else
        {

            Serial.println("setBoardName OK : ");
            (*r)["BoardName"] = getBoardName("");
        }
    }
    if (!t["getServerip"].isNull())
    {
        String ips = getPrefString(SERVER_IP_KEY, "");
        if (ips != "")
        {
            (*r)["serverip"] = ips;
        }
        else
        {
            Serial.println("getServerip error");
            ret = false;
        }
    }
    if (!t["setServerip"].isNull())
    {
        String s = t["setServerip"].as<String>();
        bool er = false;
        if (s != "")
        {

            if (setPrefString(SERVER_IP_KEY, s))
            {
                Serial.println("setServerip OK!");
                (*r)["serverip"] = s;
                er = true;
            }
            else
            {
                Serial.println("setServerip error");
            }
        }
        if (er == false)
            ret = false;
    }
    if (!t["setGateway"].isNull())
    {
        String s = t["setGateway"].as<String>();
        bool er = false;
        if (s != "")
        {

            if (setPrefString(GATWAY_IP_KEY, s))
            {
                Serial.println("setGateway OK!");
                (*r)["serverip"] = s;
                er = true;
            }
            else
            {
                Serial.print("setGateway error");
            }
        }
        if (er == false)
            ret = false;
    }
    if (!t["getGateway"].isNull())
    {
        String ips = getPrefString(GATWAY_IP_KEY, "");
        if (ips != "")
        {
            (*r)["getGateway"] = ips;
        }
        else
        {
            Serial.println("getGateway error");
            ret = false;
        }
    }
    if (!t["wifi"].isNull())
    {
        JsonArray ss = t["wifi"].as<JsonArray>();

        if (ss.size() < 2)
        {
            Serial.println("wifi param error1!");
        }
        else
        {
            String ssid = ss[0].as<String>();
            String pass = ss[1].as<String>();
            ret = Wifi_Begin(ssid.c_str(), pass.c_str());
            if (ret)
            {
                (*r)["wifi"] = WiFi.localIP().toString();
            }
        }
    }
    return ret;
}
bool FsUtils::Wifi_Begin(const char *ssid, const char *password)
{
    bool ret = false;
    if ((strlen(ssid) == 0) || (strlen(password) == 0))
    {
        Serial.println("wifi param error2!");
        return ret;
    }
    Serial.println("Wifi connecting");
    if (WiFi.status() == WL_CONNECTED)
    {
        WiFi.disconnect(true, true);
        delay(500);
    }

    WiFi.begin(ssid, password);

    int retry = 20;
    while (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("*-------------");
        delay(500);
        retry--;
        if (retry <= 0)
        {
            Serial.println("Wifi failed");
            ret = false;
            break;
        }
    }
    ret = (WiFi.status() == WL_CONNECTED);
    if (ret)
    {
        Serial.println("Wifi Connected!");
    }

    return ret;
}
bool FsUtils::setPrefULong(String key, u64_t v)
{
    bool ret = false;
    Preferences pref;

    if (pref.begin(PREF_NAMESPACE, false) == false)
        return ret;
    ret = pref.putULong64(key.c_str(), v);
    pref.end();
    return ret;
}
u64_t FsUtils::getPrefULong(String key, u64_t def)
{
    u64_t ret = def;
    Preferences pref;

    if (pref.begin(PREF_NAMESPACE, false) == false)
        return ret;
    ret = pref.getULong64(key.c_str(), def);
    pref.end();
    return ret;
}
