//
// Copyright (C) 2011 CoCo Communications
// Copyright (C) 2012 Opensim Ltd
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

import inet.common.INETDefs;
import inet.common.packet.chunk.Chunk;
import inet.networklayer.contract.ipv4.Ipv4Address;
import inet.transportlayer.common.CrcMode;

namespace inet;

enum IgmpType
{
    IGMP_MEMBERSHIP_QUERY = 0x11;
    IGMPV1_MEMBERSHIP_REPORT = 0x12;
    IGMPV2_MEMBERSHIP_REPORT = 0x16;
    IGMPV2_LEAVE_GROUP = 0x17;
    IGMPV3_MEMBERSHIP_REPORT = 0x22;
}

class IgmpMessage extends FieldsChunk
{
    IgmpType type;         // 1 byte
    uint16_t crc = 0;                   // 2 bytes
    CrcMode crcMode = CRC_MODE_UNDEFINED;  // 0 bytes, inet only
}

class IgmpQuery extends IgmpMessage
{
    type = IGMP_MEMBERSHIP_QUERY;
    Ipv4Address groupAddress;
}

class Igmpv1Query extends IgmpQuery
{
    chunkLength = B(8);
}

class Igmpv1Report extends IgmpMessage
{
    chunkLength = B(8);
    type = IGMPV1_MEMBERSHIP_REPORT;
    Ipv4Address groupAddress;
}

class Igmpv2Query extends IgmpQuery      //FIXME Igmpv1Query and Igmpv2Query differs only value of code field.
{
    chunkLength = B(8);
    simtime_t maxRespTime;      // stored in `code` in units of 1/10 second, 0.1-25.5s
}

class Igmpv2Report extends IgmpMessage
{
    chunkLength = B(8);
    type = IGMPV2_MEMBERSHIP_REPORT;
    Ipv4Address groupAddress;
}

class Igmpv2Leave extends IgmpMessage
{
    chunkLength = B(8);
    type = IGMPV2_LEAVE_GROUP;
    Ipv4Address groupAddress;
}

//
// Igmpv3Query extends Igmpv2Query, because
// Igmpv2 routers must accept Igmpv3Query packets.
//
// Note: in Igmpv3Query the maxResponseTime field is
// renamed to maxResponseCode and it is interpreted
// differently than in Igmpv2.
class Igmpv3Query extends Igmpv2Query
{
    // maxRespTime stored in `code` in units of 1/10 second
    //     0-127: 0-12.7s
    //   128-255: 1eeemmmm : (mant | 0x10) << (exp + 3)
    bool suppressRouterProc;
    unsigned char robustnessVariable;
    unsigned char queryIntervalCode;
    Ipv4AddressVector sourceList;
}

enum GroupRecordType
{
    MODE_IS_INCLUDE = 1;
    MODE_IS_EXCLUDE = 2;
    CHANGE_TO_INCLUDE_MODE = 3;
    CHANGE_TO_EXCLUDE_MODE = 4;
    ALLOW_NEW_SOURCES = 5;
    BLOCK_OLD_SOURCE = 6;
}

struct GroupRecord
{
    int recordType enum(GroupRecordType);
    Ipv4Address groupAddress;
    Ipv4AddressVector sourceList;
};

class Igmpv3Report extends IgmpMessage
{
    type = IGMPV3_MEMBERSHIP_REPORT;
    GroupRecord groupRecord[];
}

