#ifndef DSA_H
#define DSA_H
#include "models.h"

struct HashNode{
    string key;
    int val;
    HashNode* next;
    HashNode(string k,int v, Hashnode* n =nullptr): key(k),val(v), next(n)
{}};


