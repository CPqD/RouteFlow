#include "IPAddress.h"
#include "MACAddress.h"
#include <iostream>

int main() {
    cout << "Test 1" << endl;
    uint8_t data[] = {192, 168, 0, 1};
    IPAddress a(IPV4, data);
    cout << a.toString() << endl;
    cout << endl;

    cout << "Test 2" << endl;
    uint8_t array[128];
    IPAddress b(IPV6, "ce70:0000:c4ff:0000:0000:faba:0000:1234");
    b.toArray(array);
    IPAddress c(IPV6, array);
    cout << c.toString() << endl;
    cout << endl;

    cout << "Test 3" << endl;
    for (uint32_t i = 1; i < 15; i++) {
        IPAddress d(i*1234567890);
        IPAddress e(d.toUint32());
        cout << d.toString() << "\t" << e.toString() << endl;
    }
    cout << endl;

    cout << "Test 4" << endl;
    IPAddress f(IPV4, "192.168.0.1");
    IPAddress g(IPV6, "1111:2222:3333:4444:5555:6666:7777:8888");
    cout << f.toString() << "\t\t\t\t" << g.toString() << endl;
    f = g;
    cout << f.toString() << "\t" << g.toString() << endl;
    cout << endl;

    cout << "Test 5" << endl;
    MACAddress m = MACAddress("5c:26:0a:67:66:75");
    cout << m.toString() << endl;
    uint8_t mac[6];
    m.toArray(mac);
    MACAddress n(mac);
    cout << n.toString() << endl;
    cout << endl;

    cout << "Test 6" << endl;
    string address("192.168.2.10");
    IPAddress x = IPAddress(IPV4, address);
    cout << x.toString() << endl;
    cout << endl;

    cout << "Test 7" << endl;
    IPAddress y = IPAddress(IPV4, "255.255.0.0");
    cout << y.toCIDRMask() << endl;
    cout << endl;
    
    cout << "Test 8" << endl;
    IPAddress c1 = IPAddress(IPV4, "172.31.1.2");
    IPAddress c2 = IPAddress(IPV4, "172.31.1.2");
    cout << "c1 == c2? " << (c1 == c2) << " (must be 1)" << endl;
     
    return 0;
}
