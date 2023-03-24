#include "sol.h"
#include <bit>
#include "amf/deserializer.hpp"
#include "amf/types/amfstring.hpp"
#include "binary/BinaryReader.h"
#include "binary/BinaryWriter.h"

#define DEF_BYTE_STRING(name, data) std::string name((char *)data, (char *)data + sizeof(data) - 1);

DEF_BYTE_STRING(HEADER_VERSION, "\x00\xbf")
DEF_BYTE_STRING(HEADER_SIGNATURE, "TCSO\x00\x04\x00\x00\x00\x00")
DEF_BYTE_STRING(HEADER_PADDING, "\x00\x00\x00")

void Sol::load(const std::string &path) {
    static_assert(std::endian::native == std::endian::little, "endian is not little");

    filename = path;
    keys.clear();
    items.clear();

    BinaryReader reader(path);

    if (!reader.IsGood()) {
        throw std::runtime_error(std::string("Failed to read ") + path);
    }

    auto version = reader.ReadFixedLengthString(HEADER_VERSION.size());
    if (version != HEADER_VERSION) {
        throw std::exception("Unknown SOL version in header");
    }

    auto len = amf::swap_endian(reader.ReadUint32());
    auto len2 = reader.Length() - reader.Position();
    if (len != len2) {
        throw std::exception("Inconsistent stream header length");
    }

    auto signature = reader.ReadFixedLengthString(HEADER_SIGNATURE.size());
    if (signature != HEADER_SIGNATURE) {
        throw std::exception("Invalid signature");
    }

    auto nameLen = amf::swap_endian(reader.ReadUint16());
    name = reader.ReadFixedLengthString(nameLen);

    auto padding = reader.ReadFixedLengthString(HEADER_PADDING.size());
    if (padding != HEADER_PADDING) {
        throw std::exception("Invalid padding read");
    }

    if (reader.ReadInt8() != 3) {
        throw std::exception("Not amf3 format");
    }

    amf::v8 data(reader.Length() - reader.Position());
    reader.ReadToMemory(&data[0], data.size());

    amf::SerializationContext ctx;
    ctx.refEnabled = true;

    auto it = data.cbegin();
    auto end = data.cend();

    while (it != end) {
        auto name = amf::AmfString::deserializeValue(it, end, ctx);
        auto val = amf::Deserializer::deserialize(it, end, ctx);

        items[name] = std::move(val);
        keys.emplace_back(std::move(name));

        if (*it++) {
            throw std::exception("Missing padding byte");
        }
    }
}

void Sol::save(const std::string &path) {
    std::string savePath = path;
    if (path.empty()) {
        savePath = filename;
    }

    BinaryWriter writer(savePath);

    if (!writer.IsGood()) {
        throw std::runtime_error(std::string("Failed to open ") + savePath);
    }

    writer.WriteFixedLengthString(HEADER_VERSION);
    writer.WriteUint32(0);
    writer.WriteFixedLengthString(HEADER_SIGNATURE);

    uint16_t nameLen = amf::swap_endian((uint16_t)name.size());
    writer.WriteUint16(nameLen);
    writer.WriteFixedLengthString(name);

    writer.WriteFixedLengthString(HEADER_PADDING);
    writer.WriteInt8(3);

    /*
    When refEnabled is true, there are some bugs that may cause the format of the saved sol file to be incorrect
    Set refEnabled to false, although this may result in a larger sol file
    */
    amf::SerializationContext ctx;
    ctx.refEnabled = false;

    for (auto &key : keys) {
        writer.Write((amf::AmfString(key).serializeValue(ctx)));
        writer.Write(items[key]->serialize(ctx));
        writer.WriteInt8(0);
    }

    writer.SeekBeg(2);
    uint32_t len = amf::swap_endian(uint32_t(writer.Length() - 6));
    writer.WriteUint32(len);
}

amf::AmfItem &Sol::root() {
    if (items.empty()) {
        throw std::exception("no value");
    }

    return items.begin()->second.value();
}
