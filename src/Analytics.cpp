#include "../include/Graph.h"
#include <algorithm>
#include <queue>
using namespace std;
// Phase 2: Core Analytics
// finding the accounts that both user A and user B follow
vector<string> SocialNetwork::getSharedFollows(const string& userA, const string& userB) {
    string lowerA = toLower(userA);
    string lowerB = toLower(userB);
    vector<string> sharedFollows;
    
    if (userToInt.find(lowerA) == userToInt.end() || userToInt.find(lowerB) == userToInt.end()) return sharedFollows; 

    if (!isCsrValid) buildCSR();

    int idA = userToInt[lowerA];
    int idB = userToInt[lowerB];

    // CSR Logic - extracting bounds for both users
    int startA = csrRowPtr[idA], endA = csrRowPtr[idA + 1];
    int startB = csrRowPtr[idB], endB = csrRowPtr[idB + 1];

    int i = startA, j = startB;
    // Guaranteed instant intersection because adjList (and thus CSR) is sorted
    while (i < endA && j < endB) {
        if (csrColInd[i] == csrColInd[j]) {
            sharedFollows.push_back(intToUser[csrColInd[i]]);
            i++; j++;
        } else if (csrColInd[i] < csrColInd[j]) {
            i++;
        } else {
            j++;
        }
    }
    return sharedFollows;
}

// finding the shortest connection chain between two users (BFS + LRU Cache)
vector<string> SocialNetwork::getShortestPath(const string& userA, const string& userB) {
    string lowerA = toLower(userA);
    string lowerB = toLower(userB);
    string cacheKey = lowerA + "|" + lowerB; 
    
    // checking if we already did this exact search recently
    vector<string> cachedPath;
    if (shortestPathCache.get(cacheKey, cachedPath)) return cachedPath;

    if (userToInt.find(lowerA) == userToInt.end() || userToInt.find(lowerB) == userToInt.end()) return {};

    int startNode = userToInt[lowerA];
    int targetNode = userToInt[lowerB];

    if (startNode == targetNode) return {userA};
    if (!isCsrValid) buildCSR();

    vector<int> parent(nextId, -1);
    vector<bool> visited(nextId, false);
    queue<int> q;
    q.push(startNode);
    visited[startNode] = true;
    bool found = false;

    // Standard BFS traversal using the fast CSR arrays
    while (!q.empty()) {
        int current = q.front();
        q.pop();
        if (current == targetNode) { found = true; break; }

        int startIdx = csrRowPtr[current];
        int endIdx = csrRowPtr[current + 1];
        for (int i = startIdx; i < endIdx; ++i) {
            int neighbor = csrColInd[i];
            if (!visited[neighbor]) {
                visited[neighbor] = true;
                parent[neighbor] = current;
                q.push(neighbor);
            }
        }
    }

    if (!found) return {};
    
    //walking backwards from target to start to build the path string
    vector<string> path;
    int curr = targetNode;
    while (curr != -1) {
        path.push_back(intToUser[curr]);
        curr = parent[curr];
    }
    reverse(path.begin(), path.end()); 
    shortestPathCache.put(cacheKey, path);
    return path;
}

// Phase 3: Community Detection (Weakly Connected Components)
// DSU helper to group connected nodes
struct UnionFind {
    vector<int> parent;
    vector<int> size;

    UnionFind(int n) {
        parent.resize(n);
        size.resize(n, 1);
        for (int i = 0; i < n; ++i) parent[i] = i;
    }

    int find(int i) {
        if (parent[i] == i) return i;
        return parent[i] = find(parent[i]); 
    }

    void unite(int i, int j) {
        int rootI = find(i);
        int rootJ = find(j);
        
        if (rootI != rootJ) {
            if (size[rootI] < size[rootJ]) swap(rootI, rootJ);
            parent[rootJ] = rootI;
            size[rootI] += size[rootJ];
        }
    }
};

// grouping everyone into distinct network communities
vector<SocialNetwork::Community> SocialNetwork::getCommunities() {
    if (!isCsrValid) buildCSR();
    UnionFind dsu(nextId);

    // CSR Logic: iterating through all edges linearly to build the sets
    for (int u = 0; u < nextId; ++u) {
        int startIdx = csrRowPtr[u];
        int endIdx = csrRowPtr[u + 1];
        for (int i = startIdx; i < endIdx; ++i) {
            dsu.unite(u, csrColInd[i]);
        }
    }
    
    // mapping each group leader to their members
    unordered_map<int, vector<string>> rootToMembers;
    for (int i = 0; i < nextId; ++i) {
        int root = dsu.find(i);
        rootToMembers[root].push_back(intToUser[i]);
    }
    
    vector<Community> communities;
    for (auto& pair : rootToMembers) {
        communities.push_back({pair.first, pair.second, (int)pair.second.size()});
    }
    
    // sorting so the biggest communities are at the top
    sort(communities.begin(), communities.end(), [](const Community& a, const Community& b) {
        return a.size > b.size;
    });

    return communities;
}

// Phase 4: Influence Analysis (PageRank)(Min-Heap Optimized)
// calculating global PageRank to find the most influential users overall
vector<SocialNetwork::Influencer> SocialNetwork::getTopInfluencers(int topK, int iterations, double dampingFactor) {
    int n = nextId;
    if (n == 0) return {};

    if (!isCsrValid) buildCSR();

    vector<double> rank(n, 1.0 / n);
    vector<double> newRank(n, 0.0);

    // running the PageRank algorithm iteratively
    for (int iter = 0; iter < iterations; ++iter) {
        double baseRank = (1.0 - dampingFactor) / n;
        for (int i = 0; i < n; ++i) newRank[i] = baseRank;

        for (int u = 0; u < n; ++u) {
            int startIdx = csrRowPtr[u];
            int endIdx = csrRowPtr[u + 1];
            int outDegree = endIdx - startIdx;

            if (outDegree > 0) {
                // distributing influence evenly among the people this user follows
                double share = rank[u] / outDegree;
                for (int i = startIdx; i < endIdx; ++i) {
                    newRank[csrColInd[i]] += dampingFactor * share;
                }
            } else {
                // if they don't follow anyone, distribute their influence to everyone
                double share = rank[u] / n;
                for (int i = 0; i < n; ++i) newRank[i] += dampingFactor * share;
            }
        }
        rank = newRank;
    }

    this->cachedPageRank = rank; 

    // O(NlogK) MinHeap Optimization
    // using a min-heap to keep track of the top K users without fully sorting the entire array
    auto comp = [](const Influencer& a, const Influencer& b) {
        return a.score > b.score; // MinHeap keeps the smallest element at the top
    };
    priority_queue<Influencer, vector<Influencer>, decltype(comp)> minHeap(comp);

    for (int i = 0; i < n; ++i) {
        minHeap.push({intToUser[i], rank[i]});
        if (minHeap.size() > (size_t)topK) {
            minHeap.pop(); // eject the lowest score if we exceed capacity
        }
    }

    // extract from heap and reverse to get descending order
    vector<Influencer> influencers;
    while (!minHeap.empty()) {
        influencers.push_back(minHeap.top());
        minHeap.pop();
    }
    reverse(influencers.begin(), influencers.end());

    return influencers;
}

//computation for Personalized PageRank (PPR)
vector<double> SocialNetwork::computePPRVector(int sourceId, int iterations, double dampingFactor) {
    int n = nextId;
    vector<double> rank(n, 0.0);
    rank[sourceId] = 1.0; 
    vector<double> newRank(n, 0.0);

    for (int iter = 0; iter < iterations; ++iter) {
        fill(newRank.begin(), newRank.end(), 0.0);
        
        // the teleport vector is centered on the source node
        newRank[sourceId] = (1.0 - dampingFactor); 

        for (int u = 0; u < n; ++u) {
            if (rank[u] == 0.0) continue; // skipping zero-weight nodes to accelerate computation
            
            int startIdx = csrRowPtr[u];
            int endIdx = csrRowPtr[u + 1];
            int outDegree = endIdx - startIdx;

            if (outDegree > 0) {
                double share = rank[u] / outDegree;
                for (int i = startIdx; i < endIdx; ++i) {
                    newRank[csrColInd[i]] += dampingFactor * share;
                }
            } else {
                // If a node is a sink (0 out-degree), it teleports back to the source
                newRank[sourceId] += dampingFactor * rank[u];
            }
        }
        rank = newRank;
    }
    return rank;
}

// finding the most influential users relative to one specific user
vector<SocialNetwork::Influencer> SocialNetwork::getPersonalizedPageRank(const string& sourceUser, int topK, int iterations, double dampingFactor) {
    string lowerA = toLower(sourceUser);
    if (userToInt.find(lowerA) == userToInt.end()) return {};
    if (!isCsrValid) buildCSR();

    int sourceId = userToInt[lowerA];
    
    // running the PPR algorithm and push results into a min-heap to fetch the top K
    vector<double> rank = computePPRVector(sourceId, iterations, dampingFactor);
    auto comp = [](const Influencer& a, const Influencer& b) { return a.score > b.score; };
    priority_queue<Influencer, vector<Influencer>, decltype(comp)> minHeap(comp);

    for (int i = 0; i < nextId; ++i) {
        if (i == sourceId) continue;
        minHeap.push({intToUser[i], rank[i]});
        if (minHeap.size() > (size_t)topK) minHeap.pop();
    }

    vector<Influencer> influencers;
    while (!minHeap.empty()) {
        influencers.push_back(minHeap.top());
        minHeap.pop();
    }
    reverse(influencers.begin(), influencers.end());
    return influencers;
}

// suggesting new people to follow (friends of friends ranked by PR or PPR)
vector<SocialNetwork::FollowRecommendation> SocialNetwork::recommendFollows(const string& user, Strategy strategy, int topK) {
    string lowerUser = toLower(user);
    string cacheKey = lowerUser + "|" + to_string(topK) + "|" + to_string(static_cast<int>(strategy));
    
    // checking if we have recently calculated this exact recommendation
    vector<FollowRecommendation> cachedRecs;
    if (recommendationCache.get(cacheKey, cachedRecs)) return cachedRecs;

    if (userToInt.find(lowerUser) == userToInt.end()) return {};
    if (!isCsrValid) buildCSR();

    int targetId = userToInt[lowerUser];
    
    // determining which influence algorithm to use for scoring
    vector<double> activeInfluenceMap;
    if (strategy == Strategy::GlobalPageRank) {
        if (cachedPageRank.empty()) getTopInfluencers(nextId, 100, 0.85);
        activeInfluenceMap = cachedPageRank;
    } else {
        activeInfluenceMap = computePPRVector(targetId, 100, 0.85);
    }

    // finding all 2nd-degree connections (people followed by the people our target follows)
    unordered_map<int, unordered_set<int>> candidateToShared;
    int tStart = csrRowPtr[targetId], tEnd = csrRowPtr[targetId + 1];
    unordered_set<int> currentlyFollowing(csrColInd.begin() + tStart, csrColInd.begin() + tEnd);

    for (int i = tStart; i < tEnd; ++i) {
        int followeeId = csrColInd[i];
        for (int j = csrRowPtr[followeeId]; j < csrRowPtr[followeeId + 1]; ++j) {
            int fofId = csrColInd[j];
            if (fofId != targetId && currentlyFollowing.find(fofId) == currentlyFollowing.end()) {
                candidateToShared[fofId].insert(followeeId);
            }
        }
    }

    // scoring and ranking candidates using a minheap
    auto comp = [](const FollowRecommendation& a, const FollowRecommendation& b) { return a.score > b.score; };
    priority_queue<FollowRecommendation, vector<FollowRecommendation>, decltype(comp)> minHeap(comp);

    for (const auto& pair : candidateToShared) {
        int candidateId = pair.first;
        double totalScore = 0.0;
        vector<SharedFollowDetail> sharedDetails;
        for (int sharedId : pair.second) {
            double influence = activeInfluenceMap[sharedId];
            totalScore += influence;
            sharedDetails.push_back({intToUser[sharedId], influence});
        }
        minHeap.push({intToUser[candidateId], totalScore, sharedDetails});
        if (minHeap.size() > (size_t)topK) minHeap.pop();
    }

    // putting heap into a sorted list
    vector<FollowRecommendation> recommendations;
    while (!minHeap.empty()) { recommendations.push_back(minHeap.top()); minHeap.pop(); }
    reverse(recommendations.begin(), recommendations.end());

    recommendationCache.put(cacheKey, recommendations);
    return recommendations;
}

// Phase 5: Statistics and Edge Fetching
// Numbers for the dashboard
SocialNetwork::GraphStats SocialNetwork::getNetworkStatistics() {
    if (!isCsrValid) buildCSR();
    
    GraphStats stats = {0};
    stats.numUsers = nextId;
    stats.totalFollows = totalEdges;
    
    if (nextId > 0) stats.avgFollowsPerUser = (double)totalEdges / nextId;
    if (nextId > 1) stats.density = (double)totalEdges / (nextId * (nextId - 1));

    vector<int> inDegrees(nextId, 0);
    // finding maximum following and tally up incoming followers
    for (int u = 0; u < nextId; ++u) {
        int outDegree = csrRowPtr[u + 1] - csrRowPtr[u];
        if (outDegree > stats.maxFollowing) stats.maxFollowing = outDegree;
        for (int i = csrRowPtr[u]; i < csrRowPtr[u + 1]; ++i) {
            inDegrees[csrColInd[i]]++;
        }
    }

    // identifying isolated accounts and max followers
    for (int i = 0; i < nextId; ++i) {
        int outDegree = csrRowPtr[i + 1] - csrRowPtr[i];
        if (inDegrees[i] == 0 && outDegree == 0) stats.isolatedUsers++;
        if (inDegrees[i] > stats.maxFollowers) stats.maxFollowers = inDegrees[i];
    }

    // calculating community statistics
    vector<Community> comms = getCommunities();
    stats.numComponents = comms.size();
    
    int maxComm = 0, totalCommSize = 0;
    for (const auto& c : comms) {
        if (c.size > maxComm) maxComm = c.size;
        totalCommSize += c.size;
    }
    
    stats.largestCommunity = maxComm;
    if (stats.numComponents > 0) stats.avgCommunitySize = (double)totalCommSize / stats.numComponents;

    return stats;
}

// dumping out all the edges so the UI can draw the network graph
vector<pair<string, string>> SocialNetwork::getEdges() {
    if (!isCsrValid) buildCSR();
    
    vector<pair<string, string>> edges;
    for (int u = 0; u < nextId; ++u) {
        int startIdx = csrRowPtr[u];
        int endIdx = csrRowPtr[u + 1];
        for (int i = startIdx; i < endIdx; ++i) {
            edges.push_back({intToUser[u], intToUser[csrColInd[i]]});
        }
    }
    return edges;
}

// Phase 6: Fast-Path UI Snapshot
// putting everything up (PR, degrees, communities) into one flat list for the UI
// so it doesnt have to hit the engine multiple times just to draw the screen.
vector<SocialNetwork::UserSnapshot> SocialNetwork::getGraphSnapshot() {
    if (!isCsrValid) buildCSR();
    // making sure we have up to date PageRank data
    if (cachedPageRank.empty()) getTopInfluencers(nextId, 100, 0.85);

    // mapping everyone to their respective communities
    vector<Community> comms = getCommunities();
    vector<int> userToComm(nextId, 0);
    for (size_t cId = 0; cId < comms.size(); ++cId) {
        for (const string& member : comms[cId].members) {
            userToComm[userToInt[toLower(member)]] = cId;
        }
    }

    // calculating how many followers each person has
    vector<int> inDegrees(nextId, 0);
    for (int i = 0; i < csrColInd.size(); ++i) inDegrees[csrColInd[i]]++;

    vector<UserSnapshot> snapshot;
    snapshot.reserve(nextId);
    
    // assembling the final snapshot list
    for (int i = 0; i < nextId; ++i) {
        int outDegree = csrRowPtr[i + 1] - csrRowPtr[i];
        snapshot.push_back({
            intToUser[i], cachedPageRank[i], userToComm[i], inDegrees[i], outDegree
        });
    }
    return snapshot;
}