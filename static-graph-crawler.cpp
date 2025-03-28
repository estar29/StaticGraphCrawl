// Evan Stark - March 19th 2025 - ITCS 4145 001

// This program implements the previous graph crawler assignment, now using a level-by-level
// approach to print and visit all nodes in each level before going to the next one.
// This program also implements mutexes and threading to improve run-time and implement said 
// level-by-level approach.

/*----------
SOURCES USED
https://www.geeksforgeeks.org/char-vs-stdstring-vs-char-c/ (Reviewing what strings to use.)
https://www.geeksforgeeks.org/stdstring-class-in-c/?ref=ml_lbp (Differences b/w char* and std::string)
https://www.geeksforgeeks.org/strdup-strdndup-functions-c/ (Reviewing what strdndup and strdup does; string duplication.)
https://curl.se/libcurl/c/curl_easy_setopt.html (Reading the documentation for curl_easy_setopt (set option) method.)
https://curl.se/libcurl/c/libcurl-tutorial.html (Reviewing the libcurl tutorial.)
https://cplusplus.com/reference/string/string/c_str/ (Reviewing what c_str does.)
https://en.cppreference.com/w/cpp/container/vector (Reviewing std::vector documentation.)
https://www.geeksforgeeks.org/cpp-strcat/ (Overview of the strcat function.)
https://www.geeksforgeeks.org/how-to-convert-std-string-to-char-in-cpp/ (How to cast std::string to a const char*)
https://medium.com/@abhishekjainindore24/mutex-in-c-threads-part-1-45aeac3ab62d (Article reviewing threads and mutexes.)
https://www.geeksforgeeks.org/segmentation-fault-c-cpp/ (Segfault review.)
https://rapidjson.org/group___r_a_p_i_d_j_s_o_n___e_r_r_o_r_s.html#gga7d3acf640886b1f2552dc8c4cd6dea60ab707b848425668e765def25554735242 (Going through rapidjson error codes.)
------------*/

// FOR SUBMISSION: Only need to print output runtimes.
// Probably print params and output time.

// TODO: Have output actually work.
      // Implement threading.
      // Ensure mutexes work how they are supposed to.

#include <vector>
#include <string>
#include <iostream>
#include <curl/curl.h>
#include "rapidjson/document.h"
#include <mutex>

using namespace std;

// Function to pass to the write function
size_t print_data(char *buffer, size_t size, size_t nmemb, void *user_par)
{
    std::string* printed_data = (std::string*) user_par;
    for (size_t i = 0; i < size; i++) 
    {
        printed_data->push_back(buffer[i]);
    }

}

int main (int argc, char* argv[])
{
    // Initialize vectors that hold visited nodes for each level.
    std::vector<char*> done_visiting;
    std::vector<std::vector<char*> > visited_levels;
    int level = 0;
    // Getting the level limit.
    int max_level = std::atoi(argv[2]);

    // Default domain: will append the child node later.
    const char* domain = "http://hollywood-graph-crawler.bridgesuncc.org/neighbors/";

    // Getting the starting node.
    char* start = strdup(argv[1]);

    // Creating all the level arrays.
    for (int i = 0; i < max_level; i++)
    {
        std::vector<char*> new_lvl;
        visited_levels.push_back(new_lvl);
    }

    std::cout << "Number of level vectors: " << visited_levels.size() << "\n";

    // Push the initial case to the level 0 vector.
    // Should be the only node at level 0.
    std::vector<char*> zero_level = visited_levels.at(0);
    zero_level.push_back(start);

    // Assign the new changes back to the visited levels vector.
    visited_levels.at(0) = zero_level;

    // Run until the level exceeds the max_level - 1.
    // Adding minus 1 so the last child nodes do not exceed the traversal length.
    while (level <= max_level - 1) 
    {
        // Mutex creation and locking the code to prevent race conditions.
        // Using lock_guard for proper RAII and automatic locking/unlocking.
        std::mutex mtx;
        std::lock_guard<std::mutex> lock(mtx);
        
        // Create a new curl object.
        CURL *curl = curl_easy_init();

        // Check to see if curl was created correctly.
        if (!curl)
        {
            std::cout << "Error allocating curl.  Exiting function" << "\n";
            return 1;
        }
        
        // Getting the current level.
        std::vector<char*> curr_level = visited_levels.at(level);
        std::cout<<"current level size: " << curr_level.size() << "\n";

        while (curr_level.size() != 0)
        {
            // Get the front node that needs to be visited.
            char* temp = curr_level.front();

            // Erase the temp node from to visit and add it to done visiting vector.
            curr_level.erase(curr_level.begin());
            done_visiting.push_back(temp);

            // Append the temp string to the domain.
            char* temp_esc = curl_easy_escape(curl, temp, 0);
            char final_domain[256];
            strcpy(final_domain, domain);
            strcat(final_domain, temp_esc);
            // Set the options for running the specified node.
            curl_easy_setopt(curl, CURLOPT_URL, final_domain);

            std::cout<<final_domain<<"\n";

            // Gather the output.
            std::string output;     // Using std::string as shown in the libcurl tutorial video.
            // Set up data and function options.
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&output);     // Need to cast to void* due to CURLOPT_WRITEDATA
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, print_data);
            // Performing the curl.
            curl_easy_perform(curl);
            // Freeing the temp_esc call.
            curl_free(temp_esc);

            // Taking in the rapidjson data.
            using namespace rapidjson;
            Document doc;
            doc.Parse(output.c_str());

            // Ensure the json object was parsed correctly.
            // Why does this NOT work?
            if (doc.GetParseError() != 0)
            {
                std::cout << "Parsing error with code " << doc.GetParseError()<<"\n";
                std::cout << "position: " << doc.GetErrorOffset() <<"\n";
                return 1;
            }

            // Validate the doc is an object and has a member called neighbors.
            // How does this function work?
            if (!doc.IsObject() || !doc.HasMember("neighbors"))
            {
                continue;
            }

            // Create and validate the children array.
            Value& children = doc["neighbors"];
            if (!children.IsArray())
            {
                continue;
            }

            std::cout<<"Child size: "<<children.Size()<<"\n";

            // Iterating through the children/neighbors.
            for (auto& iter : children.GetArray())
            {
                // Check to see if the node has already been visited.
                for (auto& visit : done_visiting)
                {
                    if (std::string(visit) == iter.GetString())
                    {
                        children.Erase(children.Begin());
                        break;
                    }

                    else {  //delete-later
                        std::cout << children.Begin() << "\n";
                    }
                }
            }

                // After duplicates have been removed, add to the apporiate level vector.
                std::vector<char*> next_level = visited_levels.at(level + 1);

                for (auto& adding : children.GetArray())
                {
                    next_level.push_back(strdup(adding.GetString() ) );
                }
                
                // Assign the new vector back at its old position.
                visited_levels.at(level + 1) = next_level;
        }

        // Deallocate the curl variable.
        curl_easy_cleanup(curl);

        // Increment the level.
        level++;
    }

    // Print the output
    for (int i = 0; i < visited_levels.size(); i++)
    {
        std::vector<char*> curr = visited_levels.at(i);
        std::cout << "LEVEL " << i << "\n";

        for (auto& printer : curr)
        {
            std::cout << std::string(printer) << "\n";
        }
    }

}