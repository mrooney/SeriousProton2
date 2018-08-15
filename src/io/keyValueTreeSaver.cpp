#include <sp2/io/keyValueTreeSaver.h>
#include <sp2/logging.h>

namespace sp {
namespace io {

void KeyValueTreeSaver::save(string filename, KeyValueTreePtr tree)
{
    KeyValueTreeSaver saver(filename);
    
    for(const auto& it : tree->root_nodes)
    {
        saver.saveNode(it, 0);
    }
}

KeyValueTreeSaver::KeyValueTreeSaver(string filename)
{
    f = fopen(filename.c_str(), "wb");
}

KeyValueTreeSaver::~KeyValueTreeSaver()
{
    if (f)
        fclose(f);
}

void KeyValueTreeSaver::saveNode(const KeyValueTreeNode& node, int indent)
{
    if (!f)
        return;
    
    if (node.id.length() == 0)
        fprintf(f, "%*s{\n", indent, "");
    else
        fprintf(f, "%*s[%s] {\n", indent, "", node.id.c_str());
    for(const auto it : node.items)
        fprintf(f, "%*s%s: %s\n", indent + 4, "", it.first.c_str(), it.second.c_str()); //TODO: Properly handle multiline strings.
    for(const auto& it : node.child_nodes)
        saveNode(it, indent + 4);
    fprintf(f, "%*s}\n", indent, "");
}

};//namespace io
};//namespace sp
