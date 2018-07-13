/*
 * This file is part of fuse-7z-ng.
 *
 * fuse-7z-ng is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * fuse-7z-ng is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with fuse-7z-ng.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "node.h"

#include <cstring>
#include <stdexcept>

using namespace std;

const int Node::ROOT_NODE_INDEX = -1;
const int Node::NEW_NODE_INDEX = -2;
unsigned int Node::max_inode = 0u;

Node::Node (Node * parent, char const * name) :
    sname(name),
    is_dir(false),
    parent(parent),
    open_count(0),
    state(CLOSED)
{
    buffer = nullptr;
    this->name = sname.c_str();
    stat.st_ino = max_inode++;
}

Node::~Node()
{
    if (buffer)
        delete buffer;

    for (nodelist_t::iterator i = childs.begin(); i != childs.end(); i++) {
        delete i->second;
    }
    childs.clear ();
}

Node *
Node::insert (char * path)
{
    //logger << "Inserting " << path << " in " << fullname() << "..." << Logger::endl;
    char * path2 = path;
    bool dir = false;
    do {
        if (*path2 == '/') {
            dir = true;
            *path2 = '\0';
        }
    } while(*path2++);

    if (dir) {
        nodelist_t::iterator i = childs.find(path);
        if (i != childs.end())
            return i->second->insert(path2);
        // not found
        //logger << "new subdir" << path << Logger::endl;
        Node * child = new Node(this, path);
        childs[child->name] = child;
        child->is_dir = true;
        return child->insert(path2);
    }
    else {
        nodelist_t::iterator i = childs.find(path);
        if (i != childs.end()) {
            Node * child = i->second;
            //logger << "When inserting " << path << " in " << fullname() << ", found that it already exists !" << Logger::endl;
            return child;
        }
        //logger << "leaf " << path << Logger::endl;
        Node * child = new Node(this, path);
        childs[child->name] = child;
        return child;
    }
}

std::string
Node::fullname() const
{
    if (parent == nullptr) {
        return sname;
    }
    if (parent->parent == nullptr) {
        return sname;
    }
    return parent->fullname() + "/" + sname;
}

Node *
Node::find(char const * path)
{
    //logger << "Finding " << path << " from " << fullname() << Logger::endl;
    if (*path == '\0') {
        return this;
    }

    char const * path2 = path;
    bool sub = false;
    do {
        if (*path2 == '/') {
            sub = true;
            break;
        }
    } while(*path2++);

    if (sub) {
        char pouet[255];
        strncpy(pouet, path, path2-path);
        pouet[path2-path] = '\0';
        //cout << "Checking for " << pouet << " in subdirs..." << flush;
        nodelist_t::iterator i = childs.find(pouet);
        if (i != childs.end()) {
            Node * child = i->second;
            path2++;
            //cout << " ok, looking for " << path2 << " in subdir " << child->name << endl;
            return child->find(path2);
        }
        //cout << "no" << endl;
        return nullptr;
    }
    else {
        nodelist_t::iterator i = childs.find(path);
        if (i != childs.end()) {
            Node * child = i->second;
            return child;
        }
        //cerr << "Child " << path << " not found in " << fullname() << " but it is not" << endl;
        return nullptr;
    }
}

