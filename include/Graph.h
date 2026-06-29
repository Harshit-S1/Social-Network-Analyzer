#ifndef GRAPH_H
#define GRAPH_H

#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set> 
#include <string>
#include <fstream>
#include <sstream>
#include <cstdint>       
#include <algorithm>
#include <cctype>
#include "LRUCache.h" 
#include "RadixTree.h" 
using namespace std;
class SocialNetwork {
public:
    // options for how you want to rank recommendations
    enum class Strategy { GlobalPageRank, PersonalizedPageRank };

    // structs for giving results back to the frontend
    struct Community { int rootId; vector<string> members; int size; };
    struct Influencer { string name; double score; };
    struct SharedFollowDetail { string name; double contribution; };
    struct FollowRecommendation { string name; double score; vector<SharedFollowDetail> sharedFollows; };
    
    // dashboard numbers
    struct GraphStats {
        int numUsers; int totalFollows; double avgFollowsPerUser; double density;
        int numComponents; int largestCommunity; double avgCommunitySize;
        int isolatedUsers; int maxFollowers; int maxFollowing;    
    };
    
    // gathers up a user's key stats for quick UI display without heavy queries
    struct UserSnapshot {
        string name; double pageRank; int communityId; int followers; int following;
    };

private:
    // graph storage
    unordered_map<string, int> userToInt; // keys are strictly lowercase for case-insensitive lookup
    vector<string> intToUser;             // mapping values back to original
    vector<vector<int>> adjList;          // The actual graph 
    unordered_set<uint64_t> seenEdges;         // tracks duplicates super fast using 64-bit combined IDs
    
    // fast-path memory (CSR) and cached rankings
    vector<double> cachedPageRank; 
    vector<int> csrRowPtr;
    vector<int> csrColInd;
    bool isCsrValid = false; 

    // caching structures so we don't recalculate heavy paths/recs if the graph hasn't changed
    LRUCache<string, vector<string>> shortestPathCache{1000};
    LRUCache<string, vector<FollowRecommendation>> recommendationCache{1000};
    RadixTree userSearchEngine;

    int nextId = 0;
    int totalEdges = 0;

    // converts a string to lowercase so we don't treat "Alice" and "alice" as two different users
    string toLower(const string& str) const {
        string lowerStr = str;
        transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), 
                       [](unsigned char c){ return tolower(c); });
        return lowerStr;
    }

    // helpers
    int getOrAssignId(const string& userId);
    void invalidateCaches();
    void buildCSR();
    vector<double> computePPRVector(int sourceId, int iterations, double dampingFactor);

public:
    SocialNetwork() = default;

    // user management
    bool addUser(const string& user);
    bool deleteUser(const string& user);
    int follow(const string& follower, const string& followee); // Returns 0 (success), 1, 2, or 3 (errors)
    int unfollow(const string& follower, const string& followee); // Returns 0, 1, or 2
    bool loadFromCSV(const string& filepath);

    // network analysis 
    vector<string> searchUsers(const string& prefix, int limit = 5);
    vector<string> getSharedFollows(const string& userA, const string& userB);
    vector<string> getShortestPath(const string& userA, const string& userB);
    vector<Community> getCommunities();
    
    // influence and recommendation algorithms
    vector<Influencer> getTopInfluencers(int topK = 10, int iterations = 100, double dampingFactor = 0.85);
    vector<Influencer> getPersonalizedPageRank(const string& sourceUser, int topK = 10, int iterations = 100, double dampingFactor = 0.85);
    vector<FollowRecommendation> recommendFollows(const string& user, Strategy strategy = Strategy::GlobalPageRank, int topK = 3);
    
    // stats and raw data exports for the frontend
    GraphStats getNetworkStatistics();
    vector<UserSnapshot> getGraphSnapshot(); // Fetches stats for UI

    // takes all connections so the UI can easily draw the network visual
    vector<pair<string, string>> getEdges();

    int getUserCount() const;
    int getEdgeCount() const;
    void printGraph() const;
};

#endif 