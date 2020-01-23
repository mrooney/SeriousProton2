#include <sp2/io/serialization/serializer.h>
#include <sp2/logging.h>

namespace sp {
namespace io {
namespace serialization {

Serializer::Serializer(const string& filename, Mode mode)
: DataSet(*this, 0), mode(mode)
{
    if (mode == Mode::Write)
    {
        f = fopen(filename.c_str(), "wb");
    }
    if (mode == Mode::Read)
    {
        f = fopen(filename.c_str(), "rb");
        initialRead();
    }
}

Serializer::~Serializer()
{
    if (f)
    {
        if (mode == Mode::Write)
        {
            finishFile();
        }
        fclose(f);
        f = nullptr;
    }
}

int Serializer::getStringIndex(const string& s)
{
    auto it = string_table.find(s);
    if (it != string_table.end())
        return it->second;
    if (mode == Mode::Read)
        return -1;
    string_table[s] = string_table.size();
    return string_table.size() - 1;
}

void Serializer::requestWriteFrom(DataSet& dataset)
{
    if (f && mode == Mode::Write)
    {
        dataset_file_position[dataset.getId()] = ftello(f);
        dataset.writeToFile(f);
    }
}

void Serializer::finishFile()
{
    requestWriteFrom(*this);

    int64_t dataset_table_position = ftello(f);
    for(unsigned int n=0; n<dataset_file_position.size(); n++)
        writeUInt(dataset_file_position[n], f);
    
    int64_t string_table_position = ftello(f);
    std::vector<string> string_vector;
    string_vector.resize(string_table.size());
    for(auto it : string_table)
        string_vector[it.second] = it.first;
    for(auto it : string_vector)
        fwrite(it.c_str(), 1, it.length() + 1, f);
    
    fwrite(&dataset_table_position, sizeof(dataset_table_position), 1, f);
    fwrite(&string_table_position, sizeof(string_table_position), 1, f);
    int64_t version = 1;
    fwrite(&version, sizeof(version), 1, f);
    int64_t magic = *reinterpret_cast<const uint64_t*>("SP2:DATA");
    fwrite(&magic, sizeof(magic), 1, f);
}

void Serializer::initialRead()
{
    fseeko(f, sizeof(uint64_t) * -4, SEEK_END);
    off_t header_start = ftello(f);
    int64_t dataset_table_position = 0;
    int64_t string_table_position = 0;
    int64_t version = 0;
    int64_t magic = 0;
    fread(&dataset_table_position, sizeof(dataset_table_position), 1, f);
    fread(&string_table_position, sizeof(string_table_position), 1, f);
    fread(&version, sizeof(version), 1, f);
    fread(&magic, sizeof(magic), 1, f);
    if (magic != *reinterpret_cast<const uint64_t*>("SP2:DATA"))
    {
        LOG(Warning, "Magic header mismatch on deserialization. Ignoring file.");
        return;
    }
    fseeko(f, dataset_table_position, SEEK_SET);
    while(ftello(f) < string_table_position)
    {
        LOG(Debug, "dataset at:", readUInt(f));
    }
    
    string s;
    int index = 0;
    while(ftello(f) < header_start)
    {
        char c = fgetc(f);
        if (c)
        {
            s += c;
        }
        else
        {
            string_table[s] = index;
            s = "";
            index += 1;
        }
    }
    string_table[s] = index;

    fseeko(f, dataset_file_position[0], SEEK_SET);
    readFromFile(f);
}

} // namespace
} // namespace
} // namespace
