#include <pybind11/pybind11.h>
#include <pybind11/stl.h> 
#include "../include/Graph.h"

namespace py = pybind11;

PYBIND11_MODULE(network_engine, m) {
    py::enum_<SocialNetwork::Strategy>(m, "Strategy")
        .value("GlobalPageRank", SocialNetwork::Strategy::GlobalPageRank)
        .value("PersonalizedPageRank", SocialNetwork::Strategy::PersonalizedPageRank)
        .export_values();

    py::class_<SocialNetwork::Community>(m, "Community")
        .def_readonly("rootId", &SocialNetwork::Community::rootId)
        .def_readonly("members", &SocialNetwork::Community::members)
        .def_readonly("size", &SocialNetwork::Community::size);

    py::class_<SocialNetwork::Influencer>(m, "Influencer")
        .def_readonly("name", &SocialNetwork::Influencer::name)
        .def_readonly("score", &SocialNetwork::Influencer::score);

    py::class_<SocialNetwork::SharedFollowDetail>(m, "SharedFollowDetail")
        .def_readonly("name", &SocialNetwork::SharedFollowDetail::name)
        .def_readonly("contribution", &SocialNetwork::SharedFollowDetail::contribution);

    py::class_<SocialNetwork::FollowRecommendation>(m, "FollowRecommendation")
        .def_readonly("name", &SocialNetwork::FollowRecommendation::name)
        .def_readonly("score", &SocialNetwork::FollowRecommendation::score)
        .def_readonly("sharedFollows", &SocialNetwork::FollowRecommendation::sharedFollows);

    py::class_<SocialNetwork::GraphStats>(m, "GraphStats")
        .def_readonly("numUsers", &SocialNetwork::GraphStats::numUsers)
        .def_readonly("totalFollows", &SocialNetwork::GraphStats::totalFollows)
        .def_readonly("avgFollowsPerUser", &SocialNetwork::GraphStats::avgFollowsPerUser)
        .def_readonly("density", &SocialNetwork::GraphStats::density)
        .def_readonly("numComponents", &SocialNetwork::GraphStats::numComponents)
        .def_readonly("largestCommunity", &SocialNetwork::GraphStats::largestCommunity)
        .def_readonly("avgCommunitySize", &SocialNetwork::GraphStats::avgCommunitySize)
        .def_readonly("isolatedUsers", &SocialNetwork::GraphStats::isolatedUsers)
        .def_readonly("maxFollowers", &SocialNetwork::GraphStats::maxFollowers)
        .def_readonly("maxFollowing", &SocialNetwork::GraphStats::maxFollowing);

    py::class_<SocialNetwork::UserSnapshot>(m, "UserSnapshot")
        .def_readonly("name", &SocialNetwork::UserSnapshot::name)
        .def_readonly("pageRank", &SocialNetwork::UserSnapshot::pageRank)
        .def_readonly("communityId", &SocialNetwork::UserSnapshot::communityId)
        .def_readonly("followers", &SocialNetwork::UserSnapshot::followers)
        .def_readonly("following", &SocialNetwork::UserSnapshot::following);

    py::class_<SocialNetwork>(m, "SocialNetwork")
        .def(py::init<>()) 
        .def("addUser", &SocialNetwork::addUser)
        .def("follow", &SocialNetwork::follow)
        .def("unfollow", &SocialNetwork::unfollow)
        .def("deleteUser", &SocialNetwork::deleteUser)
        .def("loadFromCSV", &SocialNetwork::loadFromCSV)
        .def("searchUsers", &SocialNetwork::searchUsers, py::arg("prefix"), py::arg("limit") = 5)
        .def("getSharedFollows", &SocialNetwork::getSharedFollows)
        .def("getShortestPath", &SocialNetwork::getShortestPath)
        .def("getCommunities", &SocialNetwork::getCommunities)
        .def("getTopInfluencers", &SocialNetwork::getTopInfluencers, py::arg("topK") = 10, py::arg("iterations") = 100, py::arg("dampingFactor") = 0.85)
        .def("getPersonalizedPageRank", &SocialNetwork::getPersonalizedPageRank, py::arg("sourceUser"), py::arg("topK") = 10, py::arg("iterations") = 100, py::arg("dampingFactor") = 0.85)
        .def("recommendFollows", &SocialNetwork::recommendFollows, py::arg("user"), py::arg("strategy") = SocialNetwork::Strategy::GlobalPageRank, py::arg("topK") = 3)
        .def("getNetworkStatistics", &SocialNetwork::getNetworkStatistics)
        .def("getGraphSnapshot", &SocialNetwork::getGraphSnapshot)
        .def("getEdges", &SocialNetwork::getEdges); 
}