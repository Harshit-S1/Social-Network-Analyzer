import streamlit as st
import pandas as pd
import streamlit.components.v1 as components
from pyvis.network import Network
from streamlit_searchbox import st_searchbox 
import network_engine

# Setting up the main page layout and title
st.set_page_config(layout="wide", page_title="Social Network Intelligence")

# Caching this so we don't recreate the graph on every single UI click
@st.cache_resource
def get_engine():
    engine = network_engine.SocialNetwork()

    #filling the network with some initial users and follows
    users = ["Alice", "Bob", "Charlie", "David", "Eve", "Alan", "Alicia", "Emma", "Frank"]
    for u in users: engine.addUser(u)
    engine.follow("Alice", "Bob")
    engine.follow("Alice", "Charlie")
    engine.follow("Bob", "Charlie")
    engine.follow("Charlie", "David")
    engine.follow("Eve", "Alice")
    engine.follow("Alan", "Alice")
    engine.follow("Emma", "Alice")
    engine.follow("Frank", "Emma")
    return engine

engine = get_engine()
#sidebar menu
st.sidebar.title("Network Analytics")
st.sidebar.markdown("Live C++ Memory Binding")
page = st.sidebar.radio("Navigation", ["Dashboard & Graph", "Live Graph Editor", "Influencers", "Recommendations", "Pathfinder", "Shared Connections"])

# Helper to map stats fast
# Grabs current state and create quick lookup dictionaries
snapshot = engine.getGraphSnapshot()
stats_lookup = {u.name: u for u in snapshot}
stats_lookup_lower = {u.name.lower(): u for u in snapshot}

# Page 1: Dashboard & PyVis Graph
if page == "Dashboard & Graph":
    st.title("Network Topology & Statistics")
    
    stats = engine.getNetworkStatistics()
    # if graph is empty, telling the user to add something
    if not snapshot and stats.numUsers == 0:
        st.warning("The Network is currently empty. Go to the Live Graph Editor to add users.")
        st.stop()
        
    # Dashboard Metrics
    col1, col2, col3, col4 = st.columns(4)
    col1.metric("Total Users", stats.numUsers)
    col2.metric("Total Follows", stats.totalFollows)
    col3.metric("Max Followers", stats.maxFollowers)
    col4.metric("Communities", stats.numComponents)
    
    col5, col6, col7, col8 = st.columns(4)
    col5.metric("Largest Community", f"{stats.largestCommunity} users")
    col6.metric("Average Degree", f"{stats.avgFollowsPerUser:.2f}")
    col7.metric("Network Density", f"{stats.density:.4f}")
    col8.metric("Isolated Users", stats.isolatedUsers)
    
    st.markdown("---")
    
    edges = engine.getEdges()

    # graph is given most of the screen and small part for inspecting users 
    graph_col, inspect_col = st.columns([3, 1])
    
    with graph_col:
        #pyvis network visualization
        net = Network(height='650px', width='100%', bgcolor='#0E1117', font_color='white', directed=True)
        net.barnes_hut(gravity=-8000, central_gravity=0.3, spring_length=150)

        # avoiding division by zero if PageRank is 0 across the board
        max_pr = max([u.pageRank for u in snapshot]) if snapshot else 1.0
        if max_pr == 0: max_pr = 1.0

        # adding everyone to the visual graph
        for user in snapshot:
            pr_size = 15 + 35 * (user.pageRank / max_pr) # Dynamic Node Scaling
            tooltip = f"User: {user.name}\nFollowers: {user.followers}\nFollowing: {user.following}\nCommunity ID: {user.communityId}\nPageRank: {user.pageRank:.5f}"
            net.add_node(user.name, label=user.name, size=pr_size, group=user.communityId, title=tooltip)
        
        #lines between them
        for source, target in edges:
            net.add_edge(source, target)
            
        html_data = net.generate_html()
        components.html(html_data, height=665)
        
    with inspect_col:
        st.subheader("User Inspector")
        
        #Connecting to the backend to perform user search as typing happens
        def search_inspector(searchterm: str):
            if not searchterm: return []
            return engine.searchUsers(searchterm, 5)

        inspect_user = st_searchbox(
            search_inspector,
            key="inspector_search",
            placeholder="Search a user..."
        )
        
        # If user is picked, show their detailed stats
        if inspect_user:
            u_stat = stats_lookup.get(inspect_user)
            if u_stat:
                st.markdown(f"### {inspect_user}")
                st.markdown(f"**Community:** {u_stat.communityId}")
                st.markdown(f"**PageRank:** {u_stat.pageRank:.4f}")
                st.markdown(f"**Followers:** {u_stat.followers}")
                st.markdown(f"**Following:** {u_stat.following}")
                
                # filtering the main edges list so we don't query the backend again
                following_list = [tgt for src, tgt in edges if src.lower() == inspect_user.lower()]
                follower_list = [src for src, tgt in edges if tgt.lower() == inspect_user.lower()]
                
                with st.expander("Follows", expanded=False):
                    if following_list:
                        for f in following_list: st.markdown(f"- {f}")
                    else:
                        st.write("None")
                        
                with st.expander("Followers", expanded=False):
                    if follower_list:
                        for f in follower_list: st.markdown(f"- {f}")
                    else:
                        st.write("None")
            else:
                st.error("User data not found.")
        else:
            st.info("Search a user to view their exact connections.")

#Page 2: Live Graph Editor 
elif page == "Live Graph Editor":
    st.title("Live Engine Control")
    
    col1, col2 = st.columns(2)
    
    with col1:
        st.subheader("Manage Users")
        # form to add or remove users
        with st.form("user_management_form", clear_on_submit=True):
            new_user = st.text_input("Username:")
            c1, c2 = st.columns(2)
            add_btn = c1.form_submit_button("Add User", use_container_width=True)
            del_btn = c2.form_submit_button("Delete User", type="primary", use_container_width=True)
            
            #user creation
            if add_btn and new_user:
                if engine.addUser(new_user.strip()):
                    st.success(f"'{new_user}' added successfully")
                else:
                    st.error(f"User '{new_user}' already exists (Case-Insensitive match).")
                    
            #user deletion
            if del_btn and new_user:
                if engine.deleteUser(new_user.strip()):
                    st.success(f"'{new_user}' eradicated from memory.")
                else:
                    st.error("User does not exist.")

    with col2:
        st.subheader("Manage Connections")
        #form to link users
        with st.form("connection_form", clear_on_submit=True):
            f_source = st.text_input("Source User (Follower):")
            f_target = st.text_input("Target User (Account to Follow/Unfollow):")
            
            c3, c4 = st.columns(2)
            follow_btn = c3.form_submit_button("Follow", use_container_width=True)
            unfollow_btn = c4.form_submit_button("Unfollow", type="primary", use_container_width=True)
            
            #process follows
            if follow_btn and f_source and f_target:
                status = engine.follow(f_source.strip(), f_target.strip())
                if status == 0: st.success(f"'{f_source}' is now following '{f_target}'.")
                elif status == 1: st.error("One or both users do not exist. Create them first.")
                elif status == 2: st.warning(f"'{f_source}' is already following '{f_target}'.")
                elif status == 3: st.error("A user cannot follow themselves.")
                    
            #process unfollows
            if unfollow_btn and f_source and f_target:
                status = engine.unfollow(f_source.strip(), f_target.strip())
                if status == 0: st.success("Unfollow request processed successfully.")
                elif status == 1: st.error("One or both users do not exist.")
                elif status == 2: st.warning(f"'{f_source}' was not following '{f_target}' to begin with.")

#Page 3: Influencers
elif page == "Influencers":
    st.title("Top Influencers")
    if snapshot:
        # giving the current stats to a Pandas dataframe for easy sorting and viewing
        df = pd.DataFrame([{"User": u.name, "PageRank": u.pageRank, "Followers": u.followers, "Following": u.following} for u in snapshot])
        df = df.sort_values(by="PageRank", ascending=False).reset_index(drop=True)
        df.index = df.index + 1
        st.dataframe(df, use_container_width=True)

#Page 4: Recommendations
elif page == "Recommendations":
    st.title("Follow Recommendations")
    
    # Radix tree search for super fast autocomplete
    def search_backend_radix_tree(searchterm: str):
        if not searchterm: return []
        return engine.searchUsers(searchterm, 5)

    selected_user = st_searchbox(
        search_backend_radix_tree,
        key="user_search_box",
        placeholder="Search users (Powered by C++ Radix Tree)..."
    )
    
    st.markdown("---")
    
    if selected_user:
        st.markdown(f"### Top Picks for **{selected_user}**")

        # letting the user pick how we calculate the recommendation scores
        strategy_name = st.radio("Algorithm:", ["Global PageRank", "Personalized PageRank"], horizontal=True)
        strategy_enum = network_engine.Strategy.GlobalPageRank if strategy_name == "Global PageRank" else network_engine.Strategy.PersonalizedPageRank
        
        recs = engine.recommendFollows(selected_user, strategy_enum, 3)
        
        if not recs:
            st.info(f"{selected_user} currently has no 2nd-degree recommendations.")
        else:
            # display of the suggested users and explaining why they were recommended
            for rec in recs:
                with st.expander(f"Recommend: **{rec.name}** (Weighted Score: {rec.score:.4f})", expanded=True):
                    u_stat = stats_lookup.get(rec.name)
                    if u_stat:
                        st.markdown(f"**Followers:** {u_stat.followers} | **Following:** {u_stat.following} | **PageRank:** {u_stat.pageRank:.4f}")
                        st.markdown("---")
                    
                    st.markdown("**Shared Following Bridging You:**")
                    for reason in rec.sharedFollows:
                        st.markdown(f"- **{reason.name}** *(Contributed {reason.contribution:.4f} influence)*")
    else:
        st.info("Search and select a user to view their follow recommendations.")

#Page 5: Pathfinder
elif page == "Pathfinder":
    st.title("Shortest Follow Path")
    st.markdown("Find the shortest connection chain between two users using BFS.")
    
    with st.form("pathfinder_form"):
        col1, col2 = st.columns(2)
        start_user = col1.text_input("Start User:")
        end_user = col2.text_input("Target User:")
        find_btn = st.form_submit_button("Find Follow Chain", type="primary")
        
        if find_btn and start_user and end_user:
            #trying to find shortest route from a to b
            path = engine.getShortestPath(start_user.strip(), end_user.strip())
            st.markdown("---")
            if not path:
                st.error(f"No connection chain exists between '{start_user}' and '{end_user}'.")
            else:
                st.success("Chain Discovered")
                path_str = " ➔ ".join([f"**{node}**" for node in path])
                st.markdown(f"### {path_str}")


#Page 6: Shared Connections
elif page == "Shared Connections":
    st.title("Mutual Connections")
    st.markdown("Discover the shared accounts that two users both follow. Powered by $O(d)$ CSR array intersection.")
    
    with st.form("shared_follows_form"):
        col1, col2 = st.columns(2)
        user_a = col1.text_input("First User:")
        user_b = col2.text_input("Second User:")
        find_btn = st.form_submit_button("Find Mutuals", type="primary")
        
        if find_btn and user_a and user_b:
            uA_lower = user_a.strip().lower()
            uB_lower = user_b.strip().lower()
            
            # making sure user is not compared to themselves
            if uA_lower == uB_lower:
                st.warning("Please enter two distinct users.")
            else:
                shared = engine.getSharedFollows(user_a.strip(), user_b.strip())
                st.markdown("---")
                # Checking if the users actually exist before throwing results
                if uA_lower not in stats_lookup_lower and uB_lower not in stats_lookup_lower:
                     st.error("Both users do not exist in the network.")
                elif uA_lower not in stats_lookup_lower:
                     st.error(f"User '{user_a}' does not exist.")
                elif uB_lower not in stats_lookup_lower:
                     st.error(f"User '{user_b}' does not exist.")
                elif not shared:
                    st.info(f"'{user_a}' and '{user_b}' do not follow any of the same accounts.")
                else:
                    st.success(f"Found {len(shared)} mutual connection(s)")
                    # listing out all mutuals with their PageRank score
                    for idx, mutual in enumerate(shared):
                        m_stat = stats_lookup.get(mutual)
                        if m_stat:
                            st.markdown(f"{idx + 1}. **{mutual}** *(PageRank: {m_stat.pageRank:.4f})*")
                        else:
                            st.markdown(f"{idx + 1}. **{mutual}**")