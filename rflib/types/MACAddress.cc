#include "MACAddress.h"

MACAddress::MACAddress() {}

MACAddress::MACAddress(const char* address) {
    string saddress(address);
    data_from_string(saddress);
}

MACAddress::MACAddress(const string &address) {
    data_from_string(address);
}

MACAddress::MACAddress(const uint8_t* data) {
    memcpy(this->data, data, IFHWADDRLEN);
}

MACAddress::MACAddress(const MACAddress &other) {
    other.toArray(this->data);
}

MACAddress& MACAddress::operator=(const MACAddress &other) {
    if (this != &other) {
        other.toArray(this->data);
    }
    return *this;
}

bool MACAddress::operator==(const MACAddress &other) const {
    return memcmp(other.data, this->data, IFHWADDRLEN) == 0;
}

void MACAddress::toArray(uint8_t* array) const {
    memcpy(array, this->data, IFHWADDRLEN);
}
        
string MACAddress::toString() const {
    stringstream ss;
    ss << hex << setfill('0');
    for (int i = 0; i < IFHWADDRLEN; i++) {
        ss <<  setw(2) << (int) data[i]; 
        if (i < IFHWADDRLEN - 1)
            ss << ':';
    }
        
    return ss.str();
}

void MACAddress::data_from_string(const string &address) {
    char sc;
    int byte;
    stringstream ss(address);
    ss << hex;
    for (int i = 0; i < IFHWADDRLEN; i++) {
        ss >> byte;
        ss >> sc;
        this->data[i] = (uint8_t) byte;
    }
}
