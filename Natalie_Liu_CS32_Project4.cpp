
#include <iostream>
#include <fstream>
#include <vector>
#include <list>
#include <functional>
using namespace std;

bool findchar(string input, char c){ //returns true if the character is in the input string
    for (int i = 0; i < input.size(); i++){
        if (input[i] == c)
            return true;
    }
    return false;
}

char choosedelimiter(string input){
    char possible = '/';
    while(findchar(input,possible)) //if the possible delimiter is in the string, iterate up
        possible++;
    return possible; //this will return a delimiter i can use that won't mess with my revise function!
}

bool getInt(istream& inf, int& n)
    {
        char ch;
        if (!inf.get(ch)  ||  !isascii(ch)  ||  !isdigit(ch))
            return false;
        inf.unget();
        inf >> n;
        return true;
    }

    bool getCommand(istream& inf, char& cmd, char& delim, int& length, int& offset)
    {
        if (!inf.get(cmd))
        {
            cmd = 'x';  // signals end of file
            return true;
        }
        switch (cmd)
        {
          case '+':
            return inf.get(delim).good();
          case '#':
            {
                char ch;
                return getInt(inf, offset) && inf.get(ch) && ch == ',' && getInt(inf, length);
            }
          case '\r':
          case '\n':
            return true;
        }
        return false;
    }

class hashTable{
public:
    hashTable(): stringBits(101363){} //set the number of lists to a prime number
    
    int search(string input){
        list<pair<string,int>>& searchList = stringBits[key(input)]; //find the list to search using a key
        list<pair<string,int>>::iterator p = searchList.begin(); //set an iterator to the beginning for easy search access
        while (p!=searchList.end())
        {
            if ((*p).first == input){
                return (*p).second; //return the offset of the first occurence of the string
            }
            p++;
        }
        return -1; //this is if the string isn't in the table
    }
    
    void insert(string input, int offset){
        stringBits[key(input)].push_back(pair<string,int>(input, offset)); //just add it in at the right key
    }
    
    //accessors to lists
    list<pair<string,int>>::iterator accessoffset(string input){
        return (stringBits[key(input)]).begin(); //this returns an iterator to the beginning of the list i want to access
    }
   
    list<pair<string,int>>* accesslist(string input){ //this returns a pointer to the list i want to access
        return &stringBits[key(input)];
    }
private:
    size_t key(string input){ //hash function that gives me the key for accessing an index in the vector
        return hash<string>()(input) % stringBits.size(); //use std::hash to make a hash function that is valid, % size just confirms that i don't try to access a list in a out of bounds index
    }
    vector<list<pair<string,int>>> stringBits; //vector of lists with strings and integers
};

void createRevision(istream& fold, istream& fnew, ostream& frevision){
    hashTable oldDuplicates; //make a hash table to reference when making the revision file
    string temp; //general purpose string temp

    char ch; //parse all of new text and old text into strings for easy access
    string newtxt;
    while (fnew.get(ch)){
        newtxt += ch;
    }
    string oldtxt;
    while (fold.get(ch)){
        oldtxt += ch;
    }
    for (int i = 0; i < oldtxt.size(); i++){ //add items into hash table using key: put same strings into the same lists
        temp = oldtxt.substr(i,8);
        oldDuplicates.insert(temp, i);
    }
    
    for (int i = 0; i < newtxt.size();){ //now start checking old file and new file
        temp = newtxt.substr(i,8); //take in 8 byte strings
        if (oldDuplicates.search(temp) >= 0){  //because search returns -1 if nothing is found, if this returns positive, that means that the string does indeed already exist
            int newindex; //position in new content
            int oldindex; //position in old conten
            
            vector <int> offsets; //this holds all the offsets for a single string
            list <pair<string,int>> :: iterator p = oldDuplicates.accessoffset(temp);
            for (list<pair<string,int>>* listtotraverse = oldDuplicates.accesslist(temp); p!=(*listtotraverse).end(); p++){
                offsets.push_back(p->second);
            } //this is so that i can check what is the most efficient way to copy in items! i want to get the longest length
            
            int templength;
            int maxlength = -1; //copy down the maxlength and maxmatch index so that i can output it later
            int maxmatchindex = -1;
            for (int j = 0;j<offsets.size();j++){
                templength = 0;
                oldindex = offsets[j];
                newindex = i;
                while (newindex < newtxt.size() && offsets[j] + templength < oldtxt.size() && newtxt[newindex] == oldtxt[oldindex] ){ //this checks whether or not there is a more efficient match
                    newindex++;
                    oldindex++;
                    templength++;
                } //now that i have the length of the item used, i can compare it to my max to see whether or not i need to change offset
                if (templength > maxlength){
                    maxlength = templength;
                    maxmatchindex = offsets[j];
                }

            }
            if (maxlength > 1){
                i+=maxlength;
                frevision << "#" << maxmatchindex << "," << maxlength;
            }
            else{ //if it's one or below, it's actually probably better to just add it directly (less bytes)
                i++;
                frevision << "+/" << temp[0] << "/";
            }
        }
        else{ //this is if the string is not found in the hashtable at all
            string substring;
            substring += temp[0]; //substring keeps track of what to be output
            i++;
            while(i < newtxt.size() && oldDuplicates.search(newtxt.substr(i,8)) == -1){
                substring += newtxt[i];
                i++;
            }
            while (i > newtxt.size() - 8 && i < newtxt.size()){ //this is so i don't go out of bounds (is there a small bit of text left over in the new content after i iterate everything? this is for less than 8 character bytes that will not be in the hash table)
                substring += newtxt[i];
                i++;
            }
            char delimiter = choosedelimiter(substring); //this is to make sure / or vice versa (go down ascii table) is a good delimiter
            frevision << "+" << delimiter <<substring << delimiter;
        }

    }
}

bool revise(istream& fold, istream& frevision, ostream& fnew){
    string oldcontent;
    char temp;
    while (fold.get(temp)) //add in all the old content to a string so that i can easily print it into the right file
    {
        oldcontent += temp;
    }
    
    char cmd;
    char delim;
    int length;
    int offset;

    while (getCommand(frevision, cmd, delim, length, offset)) {
        switch (cmd) {
            case '#':{ //copy from old file
                for (int i = 0; i < length;i++) {
                    fnew << oldcontent[offset+i];
                }
                break;
            }
            case '+':{
                char toAdd; //use get command to get the delimiter already, so now all i need to do is iterate in the revision file until i reach the end of the delimiter command
                while (frevision.get(toAdd) && toAdd != delim) {
                    fnew << toAdd;
                }
                break;
            }
            case 'x':
            case '\r':
            case '\n': //do nothing
                return true;
            default:
                return false;
            }
        }
    return true;
}
