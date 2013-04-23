// Copyright (C) 2013 Internet Systems Consortium, Inc. ("ISC")
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted, provided that the above
// copyright notice and this permission notice appear in all copies.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND ISC DISCLAIMS ALL WARRANTIES WITH
// REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
// AND FITNESS.  IN NO EVENT SHALL ISC BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
// LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
// OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <config.h>
#include <asiolink/io_address.h>
#include <dhcp/dhcp6.h>
#include <dhcp/hwaddr.h>
#include <dhcp/protocol_util.h>
#include <util/buffer.h>

#include <gtest/gtest.h>

#include <netinet/ip.h>

using namespace isc;
using namespace isc::asiolink;
using namespace isc::dhcp;
using namespace isc::util;

namespace {

    /*/// @brief OptionCustomTest test class.
class OptionCustomTest : public ::testing::Test {
public:
};*/

/// The purpose of this test is to verify that the IP header checksum
/// is calculated correctly.
TEST(ProtocolUtilTest, checksum) {
    // IPv4 header to be used to calculate checksum.
    const uint8_t hdr[] = {
        0x45,                      // IP version and header length
        0x00,                      // TOS
        0x00, 0x3c,                // Total length of the IP packet.
        0x1c, 0x46,                // Identification field.
        0x40, 0x00,                // Fragmentation.
        0x40,                      // TTL
        0x06,                      // Protocol
        0x00, 0x00,                // Checksum (reset to 0x00).
        0xac, 0x10, 0x0a, 0x63,    // Source IP address.
        0xac, 0x10, 0x0a, 0x0c     // Destination IP address.
    };
    // Calculate size of the header array.
    const uint32_t hdr_size = sizeof(hdr) / sizeof(hdr[0]);
    // Get the actual checksum.
    uint16_t chksum = ~calcChecksum(hdr, hdr_size);
    // The 0xb1e6 value has been calculated by other means.
    EXPECT_EQ(0xb1e6, chksum);
    // Tested function may also take the initial value of the sum.
    // Let's set it to 2 and see whether it is included in the
    // calculation.
    chksum = ~calcChecksum(hdr, hdr_size, 2);
    // The checkum value should change.
    EXPECT_EQ(0xb1e4, chksum);
}

// The purpose of this test is to verify that the Ethernet frame header
// can be decoded correctly. In particular it verifies that the source
// HW address can be extracted from it.
TEST(ProtocolUtilTest, decodeEthernetHeader) {
    // Source HW address, 6 bytes.
    const uint8_t src_hw_addr[6] = {
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15
    };
    // Destination HW address, 6 bytes.
    const uint8_t dest_hw_addr[6] = {
        0x20, 0x31, 0x42, 0x53, 0x64, 0x75
    };

    // Prepare a buffer holding Ethernet frame header and 4 bytes of
    // dummy data.
    OutputBuffer buf(1);
    buf.writeData(dest_hw_addr, sizeof(src_hw_addr));
    buf.writeData(src_hw_addr, sizeof(src_hw_addr));
    buf.writeUint16(0x800);
    // Append dummy data. We will later check that this data is not
    // removed or corrupted when reading the ethernet header.
    buf.writeUint32(0x01020304);

    // Create a buffer with truncated ethernet frame header..
    InputBuffer in_buf_truncated(buf.getData(), buf.getLength() - 6);
    // But provide valid packet object to make sure that the function
    // under test does not throw due to NULL pointer packet.
    Pkt4Ptr pkt(new Pkt4(DHCPDISCOVER, 0));
    // Function should throw because header data is truncated.
    EXPECT_THROW(decodeEthernetHeader(in_buf_truncated, pkt),
                 InvalidPacketHeader);

    // Get not truncated buffer.
    InputBuffer in_buf(buf.getData(), buf.getLength());
    // But provide NULL packet object instead.
    pkt.reset();
    // It should throw again but a different exception.
    EXPECT_THROW(decodeEthernetHeader(in_buf, pkt),
                 BadValue);
    // Now provide, correct data.
    pkt.reset(new Pkt4(DHCPDISCOVER, 0));
    // It should not throw now.
    ASSERT_NO_THROW(decodeEthernetHeader(in_buf, pkt));
    // Verify that the HW address of the source has been initialized.
    HWAddrPtr hwaddr = pkt->getHWAddr();
    ASSERT_TRUE(hwaddr);
    // And that it is correct.
    EXPECT_EQ(HWTYPE_ETHERNET, hwaddr->htype_);
    ASSERT_EQ(sizeof(dest_hw_addr), hwaddr->hwaddr_.size());
    EXPECT_TRUE(std::equal(src_hw_addr, src_hw_addr + sizeof(src_hw_addr),
                           hwaddr->hwaddr_.begin()));
    // The entire ethernet packet header should have been read. This means
    // that the internal buffer pointer should now point to its tail.
    ASSERT_EQ(ETHERNET_HEADER_LEN, in_buf.getPosition());
    // And the dummy data should be still readable and correct.
    uint32_t dummy_data = in_buf.readUint32();
    EXPECT_EQ(0x01020304, dummy_data);
}

/// The purpose of this test is to verify that the ethernet
/// header is correctly constructed from the destination and
/// hardware addresses.
TEST(ProtocolUtilTest, writeEthernetHeader) {
    // Source HW address, 6 bytes.
    const uint8_t src_hw_addr[6] = {
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15
    };
    // Destination HW address, 6 bytes.
    const uint8_t dest_hw_addr[6] = {
        0x20, 0x31, 0x42, 0x53, 0x64, 0x75
    };
    // Create output buffer.
    OutputBuffer buf(1);

    // Construct the ethernet header using HW addresses.
    writeEthernetHeader(src_hw_addr, dest_hw_addr, buf);

    // The resulting ethernet header consists of destination
    // and src HW address (each 6 bytes long) and two bytes
    // of encapsulated protocol type.
    ASSERT_EQ(14, buf.getLength());

    // Verify that first 6 bytes comprise valid destination
    // HW address. Instead of using memory comparison functions
    // we check bytes one-by-one. In case of mismatch we will
    // get exact values that are mismatched. If memcmp was used
    // the error message would not indicate the values of
    // mismatched bytes.
    for (int i = 0; i < 6; ++i) {
        EXPECT_EQ(dest_hw_addr[i], buf[i]);
    }
    // Verify that following 6 bytes comprise the valid source
    // HW address.
    for (int i = 0; i < 6; ++i) {
        EXPECT_EQ(src_hw_addr[i], buf[i + 6]);
    }

    // The last two bytes comprise the encapsulated protocol type.
    // We expect IPv4 protocol type which is specified by 0x0800.
    EXPECT_EQ(0x08, buf[12]);
    EXPECT_EQ(0x0, buf[13]);
}

TEST(ProtocolUtilTest, writeIpUdpHeader) {
    // Create DHCPv4 packet. Some values held by this object are
    // used by the utility function under test to figure out the
    // contents of the IP and UDP headers, e.g. source and
    // destination IP address or port number.
    Pkt4Ptr pkt(new Pkt4(DHCPOFFER, 0));
    ASSERT_TRUE(pkt);

    // Set local and remote address and port.
    pkt->setLocalAddr(IOAddress("192.0.2.1"));
    pkt->setRemoteAddr(IOAddress("192.0.2.111"));
    pkt->setLocalPort(DHCP4_SERVER_PORT);
    pkt->setRemotePort(DHCP4_CLIENT_PORT);

    // Pack the contents of the packet.
    ASSERT_NO_THROW(pkt->pack());

    // Create output buffer. The headers will be written to it.
    OutputBuffer buf(1);
    // Write some dummy data to the buffer. We will check that the
    // function under test appends to this data, not overrides it.
    buf.writeUint16(0x0102);

    // Write both IP and UDP headers.
    writeIpUdpHeader(pkt, buf);

    // The resulting size of the buffer must be 30. The 28 bytes are
    // consumed by the IP and UDP headers. The other 2 bytes are dummy
    // data at the beginning of the buffer.
    ASSERT_EQ(30, buf.getLength());

    // Make sure that the existing data in the buffer was not corrupted
    // by the function under test.
    EXPECT_EQ(0x01, buf[0]);
    EXPECT_EQ(0x02, buf[1]);

    // Copy the contents of the buffer to InputBuffer object. This object
    // exposes convenient functions for reading.
    InputBuffer in_buf(buf.getData(), buf.getLength());

    // Check dummy data.
    uint16_t dummy_data = in_buf.readUint16();
    EXPECT_EQ(0x0102, dummy_data);

    // The IP version and IHL are stored in the same octet (4 bits each).
    uint8_t ver_len = in_buf.readUint8();
    // The most significant bits define IP version.
    uint8_t ip_ver = ver_len >> 4;
    EXPECT_EQ(4, ip_ver);
    // The least significant bits define header length (in 32-bits chunks).
    uint8_t ip_len = ver_len & 0x0F;
    EXPECT_EQ(5, ip_len);

    // Get Differentiated Services Codepoint and Explicit Congestion
    // Notification field.
    uint8_t dscp_ecn = in_buf.readUint8();
    EXPECT_EQ(IPTOS_LOWDELAY, dscp_ecn);

    // Total length of IP packet. Includes UDP header and payload.
    uint16_t total_len = in_buf.readUint16();
    EXPECT_EQ(28 + pkt->getBuffer().getLength(), total_len);

    // Identification field.
    uint16_t ident = in_buf.readUint16();
    EXPECT_EQ(0, ident);

    // Fragmentation.
    uint16_t fragment = in_buf.readUint16();
    // Setting second most significant bit means no fragmentation.
    EXPECT_EQ(0x4000, fragment);

    // Get TTL
    uint8_t ttl = in_buf.readUint8();
    // Expect non-zero TTL.
    EXPECT_GE(ttl, 1);

    // Protocol type is UDP.
    uint8_t proto = in_buf.readUint8();
    EXPECT_EQ(IPPROTO_UDP, proto);

    // Check that the checksum is correct. The reference checksum value
    // has been calculated manually.
    uint16_t ip_checksum = in_buf.readUint16();
    EXPECT_EQ(0x755c, ip_checksum);

    // Validate source address.
    // Initializing it to IPv6 address guarantees that it is not initialized
    // to the value that we expect to be read from a header since the value
    // read from a header will be IPv4.
    IOAddress src_addr("::1");
    // Read src address as an array of bytes because it is easely convertible
    // to IOAddress object.
    uint8_t src_addr_data[4];
    ASSERT_NO_THROW(
        in_buf.readData(src_addr_data, 4);
        src_addr = IOAddress::fromBytes(AF_INET, src_addr_data);
    );
    EXPECT_EQ(IOAddress("192.0.2.1").toText(), src_addr.toText());

    // Validate destination address.
    IOAddress dest_addr("::1");
    uint8_t dest_addr_data[4];
    ASSERT_NO_THROW(
        in_buf.readData(dest_addr_data, 4);
        dest_addr = IOAddress::fromBytes(AF_INET, dest_addr_data);
    );
    EXPECT_EQ(IOAddress("192.0.2.111").toText(), dest_addr.toText());

    // UDP header starts here.

    // Check source port.
    uint16_t src_port = in_buf.readUint16();
    EXPECT_EQ(pkt->getLocalPort(), src_port);

    // Check destination port.
    uint16_t dest_port = in_buf.readUint16();
    EXPECT_EQ(pkt->getRemotePort(), dest_port);

    // UDP header and data length.
    uint16_t udp_len = in_buf.readUint16();
    EXPECT_EQ(8 + pkt->getBuffer().getLength(), udp_len);

    // Verify UDP checksum. The reference checksum has been calculated manually.
    uint16_t udp_checksum = in_buf.readUint16();
    EXPECT_EQ(0x8817, udp_checksum);
}

} // anonymous namespace
