#include "../include/Graph.h"
using namespace std;

// obtaining a user's internal ID, but first have to register them if they are brand new
int SocialNetwork::getOrAssignId(const  string& userId) {
     string lowerId = toLower(userId);
    auto it = userToInt.find(lowerId);
    if (it != userToInt.end()) return it->second;

    int assignedId = nextId;
    userToInt[lowerId] = assignedId;
    intToUser.push_back(userId); 
    adjList.push_back( vector<int>());
    userSearchEngine.insert(userId, assignedId); 
    
    nextId++;
    return assignedId;
}

// flushing all cached math and paths so we don't serve stale data after a graph change
void SocialNetwork::invalidateCaches() {
    cachedPageRank.clear();
    isCsrValid = false;
    shortestPathCache.clear();
    recommendationCache.clear();
}

// adding a new user (returns false if they already exist)
bool SocialNetwork::addUser(const  string& user) {
     string lowerUser = toLower(user);
    if (userToInt.find(lowerUser) != userToInt.end()) return false;
    getOrAssignId(user);
    return true;
}

// delete a user from the graph
// This is a heavy operation because we have to 
// shift all the internal numeric IDs down to fill the empty gap.
bool SocialNetwork::deleteUser(const  string& user) {
     string lowerUser = toLower(user);
    auto it = userToInt.find(lowerUser);
    if (it == userToInt.end()) return false;

    int idToRemove = it->second;
    userToInt.erase(it);
    intToUser.erase(intToUser.begin() + idToRemove);
    
    // wiping their outgoing connections
    totalEdges -= adjList[idToRemove].size();
    adjList.erase(adjList.begin() + idToRemove);
    seenEdges.clear(); 
    totalEdges = 0;

    // shifting all higher IDs down by 1 so our arrays stay perfectly packed
    for (auto& pair : userToInt) {
        if (pair.second > idToRemove) pair.second--;
    }

    // erasing the deleted user from everyone else's follow lists
    for (int u = 0; u < adjList.size(); ++u) {
        auto& edges = adjList[u];
        edges.erase( remove(edges.begin(), edges.end(), idToRemove), edges.end());
        for (int& v : edges) if (v > idToRemove) v--;
        for(int v : edges) {
            seenEdges.insert((static_cast<uint64_t>(u) << 32) | static_cast<uint64_t>(v));
            totalEdges++;
        }
    }
    
    nextId--;
    invalidateCaches();
    return true;
}

// linking two users together
int SocialNetwork::follow(const  string& follower, const  string& followee) {
     string lowerA = toLower(follower);
     string lowerB = toLower(followee);
    if (lowerA == lowerB) return 3; // preventing self follow

    auto itA = userToInt.find(lowerA);
    auto itB = userToInt.find(lowerB);
    if (itA == userToInt.end() || itB == userToInt.end()) return 1; // user missing

    int idA = itA->second;
    int idB = itB->second;

    // packing both 32-bit IDs into a single 64-bit int for ultra-fast duplicate checking
    uint64_t edgeSig = (static_cast<uint64_t>(idA) << 32) | static_cast<uint64_t>(idB);
    if (seenEdges.find(edgeSig) != seenEdges.end()) return 2; // Already following

    seenEdges.insert(edgeSig);
    
    // inserting the new followee while keeping the adjacency list sorted
    auto pos =  lower_bound(adjList[idA].begin(), adjList[idA].end(), idB);
    adjList[idA].insert(pos, idB);
    totalEdges++;
    invalidateCaches();
    return 0; 
}

// breaking a connection between two users
int SocialNetwork::unfollow(const  string& follower, const  string& followee) {
     string lowerA = toLower(follower);
     string lowerB = toLower(followee);
    
    auto itA = userToInt.find(lowerA);
    auto itB = userToInt.find(lowerB);
    if (itA == userToInt.end() || itB == userToInt.end()) return 1; // user missing

    int idA = itA->second;
    int idB = itB->second;

    uint64_t edgeSig = (static_cast<uint64_t>(idA) << 32) | static_cast<uint64_t>(idB);
    if (!seenEdges.erase(edgeSig)) return 2; // was not following

    // removing them from the adjacency list
    adjList[idA].erase( remove(adjList[idA].begin(), adjList[idA].end(), idB), adjList[idA].end());
    totalEdges--;
    invalidateCaches();
    return 0; 
}

// loading a graph from a comma separated file
bool SocialNetwork::loadFromCSV(const  string& filepath) {
     ifstream file(filepath);
    if (!file.is_open()) return false;
     string line;
    while ( getline(file, line)) {
        if (line.empty()) continue;
         stringstream ss(line);
         string follower, followee;
        if ( getline(ss, follower, ',') &&  getline(ss, followee, ',')) {
            // silently created if they don't exist in CSV load
            getOrAssignId(follower); 
            getOrAssignId(followee);
            follow(follower, followee);
        }
    }
    file.close();
    return true;
}

// using the Radix Trie for instant user autocomplete
 vector< string> SocialNetwork::searchUsers(const  string& prefix, int limit) {
    return userSearchEngine.searchPrefix(prefix, limit);
}

int SocialNetwork::getUserCount() const { return nextId; }
int SocialNetwork::getEdgeCount() const { return totalEdges; }
void SocialNetwork::printGraph() const {}

// Memory Compilation (CSR)
// Converting the standard 2D vector graph into flat 1D arrays
// This format makes memory access highly contiguous, running graph algorithms super fast
void SocialNetwork::buildCSR() {
    if (isCsrValid) return; // Prevent redundant builds
    
    // resizing the row pointer array (N+1)
    csrRowPtr.assign(nextId + 1, 0);
    csrColInd.clear();
    csrColInd.reserve(totalEdges); // optimization - Reserve exact memory needed

    // flattening the 2D adjacency list into the 1D Compressed Sparse Row arrays
    for (int u = 0; u < nextId; ++u) {
        csrRowPtr[u] = csrColInd.size();
        for (int v : adjList[u]) {
            csrColInd.push_back(v);
        }
    }
    // setting the final boundary pointer
    csrRowPtr[nextId] = csrColInd.size();
    
    isCsrValid = true;
}