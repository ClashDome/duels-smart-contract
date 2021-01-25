#pragma once

#include <eosio/asset.hpp>
#include <eosio/eosio.hpp>
#include <eosio/system.hpp>

using eosio::asset;
using eosio::name;
using eosio::symbol;
using eosio::symbol_code;
using eosio::check;
using eosio::permission_level;
using eosio::current_time_point;
using eosio::action;
using eosio::same_payer;
using eosio::multi_index;
using std::string;
using std::vector;
using std::make_tuple;
using namespace std;

#define CONTRACTN name("clashdomedls")

class [[eosio::contract]] clashdomedls : public eosio::contract
{
    public:

        using contract::contract;

        ACTION createduel(string state, string type, string game, asset fee);
        ACTION updateelo(string game, name winner, name loser);
        ACTION eraseall();

    private:

        struct player_duel
        {
            name account;   // user wax account
            uint64_t initial_timestamp;
            uint64_t score; // user score
        };

        struct game_info {
            string id; 
            uint64_t total_duels;  
            uint64_t wins;
            uint64_t loses;
            uint64_t MMR;
        };

        TABLE player {
            name account;
            vector<game_info> games;
            uint64_t primary_key() const { return account.value; }
        };

        typedef multi_index<name("players"), player> players;

        TABLE duel {
            uint64_t id;
            uint64_t timestamp; // duel creation time
            string type;  // public || private
            string state;  // open || compromised || closed || claimed
            string game; // endless-siege, candy-fiesta, etc
            asset fee; // entry fee for the duel
            player_duel player1;
            player_duel player2;

            uint64_t primary_key() const { return id; }
        };

        typedef multi_index<name("duels"), duel> duels;

        uint64_t finder(vector<game_info> games, string id);
};