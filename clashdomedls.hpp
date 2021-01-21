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

        ACTION createduel(string state, string type, string game);

    private:

        TABLE user {
            string user; 
            uint64_t total_duels;  
            uint64_t wins;
            uint64_t loses;
            uint64_t MMR;
            string primary_key() const { return user; }
        };

        typedef multi_index<name("users"), user> users;

        TABLE duel {
            uint64_t id; 
            string state; // open || closed  
            string type;  // public || private
            string game; // endless-siege, candy-fiesta, etc

            uint64_t primary_key() const { return id; }
        };

        typedef multi_index<name("duels"), duel> duels;
};