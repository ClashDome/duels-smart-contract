#include "clashdomedls.hpp"
#include <math.h>

void clashdomedls::create(uint64_t id, uint64_t type, uint64_t game, asset fee, uint64_t duration, string data)
{
    require_auth(_self);

    duels _dl(CONTRACTN, CONTRACTN.value);

    auto dl_itr = _dl.find(id);
    check(dl_itr == _dl.end(), "Duel with id " + to_string(id) + " already exist!");

    uint64_t timestamp = eosio::current_time_point().sec_since_epoch();

    player_duel player;

    player.timestamp = timestamp;
    player.duration = duration;
    player.data = data;

    _dl.emplace(CONTRACTN, [&](auto& new_duel) {
        new_duel.id = id;
        new_duel.timestamp = timestamp;
        new_duel.state = DuelState::OPEN;
        new_duel.type = type;
        new_duel.game = game;
        new_duel.fee = fee;
        new_duel.player1 = player;
    });
}

void clashdomedls::create2(uint64_t id, uint64_t type, uint64_t game, asset fee, uint64_t duration, string data, name account)
{
    require_auth(_self);

    duels _dl(CONTRACTN, CONTRACTN.value);

    auto dl_itr = _dl.find(id);
    check(dl_itr == _dl.end(), "Duel with id " + to_string(id) + " already exist!");

    checkPayments(game, fee, account);

    uint64_t timestamp = eosio::current_time_point().sec_since_epoch();

    player_duel player;

    player.timestamp = timestamp;
    player.duration = duration;
    player.data = data;

    _dl.emplace(CONTRACTN, [&](auto& new_duel) {
        new_duel.id = id;
        new_duel.timestamp = timestamp;
        new_duel.state = DuelState::OPEN;
        new_duel.type = type;
        new_duel.game = game;
        new_duel.fee = fee;
        new_duel.player1 = player;
    });
}

void clashdomedls::compromise(uint64_t id, name account)
{
    require_auth(_self);

    duels _dl(CONTRACTN, CONTRACTN.value);

    auto dl_itr = _dl.find(id);
    check(dl_itr != _dl.end(), "Duel with id " + to_string(id) + " doesn't exist!");
    check(dl_itr->state != DuelState::COMPROMISED, "Duel with id " + to_string(id) + " already compromised!");
    check(dl_itr->state == DuelState::OPEN, "Duel with id " + to_string(id) + " can't be compromised!");

    uint64_t timestamp = eosio::current_time_point().sec_since_epoch();
    
    player_duel player;

    player.account = account;
    player.timestamp = timestamp;

    _dl.modify(dl_itr, get_self(), [&](auto &mod_duel) {
        mod_duel.state = DuelState::COMPROMISED;
        mod_duel.player2 = player;
    });
}

void clashdomedls::close(uint64_t id, name account, uint64_t score, uint64_t duration, uint64_t score2, name account2)
{
    require_auth(_self);

    duels _dl(CONTRACTN, CONTRACTN.value);

    auto dl_itr = _dl.find(id);
    check(dl_itr != _dl.end(), "Duel with id " + to_string(id) + " doesn't exist!");
    check(dl_itr->state != DuelState::CLOSED, "Duel with id " + to_string(id) + " already closed!");
    check(dl_itr->state == DuelState::COMPROMISED, "Duel with id " + to_string(id) + " can't be closed!");
    check(dl_itr->player2.account == account, "Second player account mismatch!");

    checkPayments(dl_itr->game, dl_itr->fee, account);

    uint64_t timestamp = eosio::current_time_point().sec_since_epoch();

    _dl.modify(dl_itr, get_self(), [&](auto &mod_duel) {
        mod_duel.state = DuelState::CLOSED;
        mod_duel.timestamp = timestamp;
        mod_duel.player1.score = score2;
        mod_duel.player1.account = account2;
        mod_duel.player1.data = "";
        mod_duel.player2.score = score;
        mod_duel.player2.duration = duration;
    });

    // update elo

    name winner;
    name loser;
    uint64_t game = dl_itr->game;

    if (dl_itr->player1.score > dl_itr->player2.score) {
        winner = dl_itr->player1.account;
        loser = dl_itr->player2.account;
    } else if (dl_itr->player2.score > dl_itr->player1.score) {
        winner = dl_itr->player2.account;
        loser = dl_itr->player1.account;
    } else {
        if (dl_itr->player1.duration <= dl_itr->player2.duration) {
            winner = dl_itr->player1.account;
            loser = dl_itr->player2.account;
        } else {
            winner = dl_itr->player2.account;
            loser = dl_itr->player1.account;
        }
    }

    players _pl(CONTRACTN, CONTRACTN.value);

    // winner
    auto pl_itr = _pl.find(winner.value);

    // new user
    if (pl_itr == _pl.end()) {

        vector<game_info> games;
        games.push_back({ game, 1, 1, 0, 1000}); 

        _pl.emplace(CONTRACTN, [&](auto& new_player) {
            new_player.account = winner;
            new_player.games = games;
        });
    } else {

        uint64_t pos = finder(pl_itr->games, game);

        // new game
        if (pos == -1) {
            _pl.modify(pl_itr, get_self(), [&](auto &mod_player) {
                mod_player.games.push_back({ game, 1, 1, 0, 1000 });
            });
        } else {
            _pl.modify(pl_itr, get_self(), [&](auto &mod_player) {
                mod_player.games.at(pos).total_duels++;
                mod_player.games.at(pos).wins++;
            });
        }
    }

    // loser
    auto pl_itr_2 = _pl.find(loser.value);

    // new user
    if (pl_itr_2 == _pl.end()) {

        vector<game_info> games;
        games.push_back({ game, 1, 0, 1, 1000}); 

        _pl.emplace(CONTRACTN, [&](auto& new_player) {
            new_player.account = loser;
            new_player.games = games;
        });
    } else {

        uint64_t pos = finder(pl_itr_2->games, game);

        // new game
        if (pos == -1) {
            _pl.modify(pl_itr_2, get_self(), [&](auto &mod_player) {
                mod_player.games.push_back({ game, 1, 0, 1, 1000 });
            });
        } else {
            _pl.modify(pl_itr_2, get_self(), [&](auto &mod_player) {
                mod_player.games.at(pos).total_duels++;
                mod_player.games.at(pos).loses++;
            });
        }
    }

    pl_itr = _pl.find(winner.value);
    uint64_t pos = finder(pl_itr->games, game);
    uint64_t winnerMMR = pl_itr->games.at(pos).MMR;

    pl_itr_2 = _pl.find(loser.value);
    uint64_t pos2 = finder(pl_itr_2->games, game);
    uint64_t loserMMR = pl_itr_2->games.at(pos2).MMR;

    double p1 = 1.0 / (1.0 + pow(10.0, (loserMMR - winnerMMR) * 0.0025)); 
    double p2 = 1.0 - p1;
    uint64_t K = 30;

    winnerMMR = winnerMMR + K * (1.0 - p1);
    loserMMR = loserMMR + K * (0.0 - p2);

    _pl.modify(pl_itr, get_self(), [&](auto &mod_player) {
        mod_player.games.at(pos).MMR = winnerMMR;
    });

    _pl.modify(pl_itr_2, get_self(), [&](auto &mod_player) {
        mod_player.games.at(pos2).MMR = loserMMR;
    });
}

void clashdomedls::claim(uint64_t id, name account)
{
    require_auth(account);

    duels _dl(CONTRACTN, CONTRACTN.value);

    auto dl_itr = _dl.find(id);
    check(dl_itr != _dl.end(), "Duel with id " + to_string(id) + " doesn't exist!");
    check(dl_itr->state != DuelState::CLAIMED, "Duel with id " + to_string(id) + " already claimed!");
    check(dl_itr->state == DuelState::CLOSED, "Duel with id " + to_string(id) + " can't be claimed yet!");

    name winner;

    if (dl_itr->player1.score > dl_itr->player2.score) {
        winner = dl_itr->player1.account;
    } else if (dl_itr->player2.score > dl_itr->player1.score) {
        winner = dl_itr->player2.account;
    } else {
        if (dl_itr->player1.duration <= dl_itr->player2.duration) {
            winner = dl_itr->player1.account;
        } else {
            winner = dl_itr->player2.account;
        }
    }

    check(winner == account, "Player with account " + account.to_string() + " isn't the winner!");

    _dl.modify(dl_itr, get_self(), [&](auto &mod_duel) {
        mod_duel.state = DuelState::CLAIMED;
    });

    string gameString = "";

    if (dl_itr->game == GameType::ENDLESS_SIEGE) {
        gameString = "Endless Siege";
    } else if (dl_itr->game == GameType::CANDY_FIESTA) {
        gameString = "Candy Fiesta";
    } else {
        gameString = "Non Existing Game";
    }

    action(permission_level{_self, "active"_n}, EOS_CONTRACT, "transfer"_n, make_tuple(_self, account, dl_itr->fee * 190 / 100, string(gameString + ". Duel id " + to_string(id) + " - Winner"))).send(); 
    // TODO: change this for production mode 
    // action(permission_level{_self, "active"_n}, EOS_CONTRACT, "transfer"_n, make_tuple(_self, "clashdometkn"_n, dl_itr->fee * 10 / 100, string(gameString + ". Duel id " + to_string(id) + " - Commission"))).send();  
    action(permission_level{_self, "active"_n}, EOS_CONTRACT, "transfer"_n, make_tuple(_self, COMPANY_ACCOUNT, dl_itr->fee * 10 / 100, string(gameString + ". Duel id " + to_string(id) + " - Commission"))).send();  

    asset ludio;
    ludio.symbol = LUDIO_SYMBOL;

    ludio.amount = dl_itr->fee.amount * WAX_TO_LUDIO_RATIO * 0.0001; // decimal conversion LUDIO 4 decimals, WAX 8 decimals

    action(permission_level{_self, "active"_n}, LUDIO_CONTRACT, "transfer"_n, make_tuple(_self, account, ludio, string(gameString + ". Duel id " + to_string(id) + " - Winner extra Ludio"))).send(); 

    // remove first duel 
    auto dl_itr_rem = _dl.begin();

    if (dl_itr_rem->state == DuelState::CLAIMED) {
        _dl.erase(dl_itr_rem); 
    }
        
}

void clashdomedls::forceclaim(uint64_t id)
{
    require_auth(_self);

    duels _dl(CONTRACTN, CONTRACTN.value);

    auto dl_itr = _dl.find(id);

    check(dl_itr != _dl.end(), "Duel with id " + to_string(id) + " doesn't exist!");
    check(dl_itr->state == DuelState::CLOSED, "Duel with id " + to_string(id) + " can't be claimed!");

    name winner;

    if (dl_itr->player1.score > dl_itr->player2.score) {
        winner = dl_itr->player1.account;
    } else if (dl_itr->player2.score > dl_itr->player1.score) {
        winner = dl_itr->player2.account;
    } else {
        if (dl_itr->player1.duration <= dl_itr->player2.duration) {
            winner = dl_itr->player1.account;
        } else {
            winner = dl_itr->player2.account;
        }
    }

    _dl.modify(dl_itr, get_self(), [&](auto &mod_duel) {
        mod_duel.state = DuelState::CLAIMED;
    });

    string gameString = "";

    if (dl_itr->game == GameType::ENDLESS_SIEGE) {
        gameString = "Endless Siege";
    } else if (dl_itr->game == GameType::CANDY_FIESTA) {
        gameString = "Candy Fiesta";
    } else {
        gameString = "Non Existing Game";
    }

    // multiply by 1.9 instead of 190 / 100
    action(permission_level{_self, "active"_n}, EOS_CONTRACT, "transfer"_n, make_tuple(_self, winner, dl_itr->fee * 190 / 100, string(gameString + ". Duel id " + to_string(id) + " - Winner"))).send(); 
    // TODO: change this for production mode 
    // action(permission_level{_self, "active"_n}, EOS_CONTRACT, "transfer"_n, make_tuple(_self, "clashdometkn"_n, dl_itr->fee * 10 / 100, string(gameString + ". Duel id " + to_string(id) + " - Commission"))).send();  
    action(permission_level{_self, "active"_n}, EOS_CONTRACT, "transfer"_n, make_tuple(_self, COMPANY_ACCOUNT, dl_itr->fee * 10 / 100, string(gameString + ". Duel id " + to_string(id) + " - Commission"))).send();  

    asset ludio;
    ludio.symbol = LUDIO_SYMBOL;

    ludio.amount = dl_itr->fee.amount * WAX_TO_LUDIO_RATIO * 0.0001; // decimal conversion LUDIO 4 decimals, WAX 8 decimals

    action(permission_level{_self, "active"_n}, LUDIO_CONTRACT, "transfer"_n, make_tuple(_self, winner, ludio, string(gameString + ". Duel id " + to_string(id) + " - Winner extra Ludio"))).send();
    
    // remove first duel 
    auto dl_itr_rem = _dl.begin();

    if (dl_itr_rem->state == DuelState::CLAIMED) {
        _dl.erase(dl_itr_rem); 
    }

    // action(
    //     permission_level{get_self(), name("active")},
    //     get_self(),
    //     name("logremove"),
    //     std::make_tuple(
    //         dl_itr_rem->id
    //     )
    // ).send();   
}

void clashdomedls::reopen(uint64_t id)
{
    require_auth(_self);

    duels _dl(CONTRACTN, CONTRACTN.value);

    auto dl_itr = _dl.find(id);
    check(dl_itr != _dl.end(), "Duel with id " + to_string(id) + " doesn't exist!");
    check(dl_itr->state == DuelState::COMPROMISED, "Duel with id " + to_string(id) + " can't be reopened!");

    player_duel player;

    player.timestamp = 0;

    _dl.modify(dl_itr, get_self(), [&](auto &mod_duel) {
        mod_duel.state = DuelState::OPEN;
        mod_duel.player2 = player;
    });
}

void clashdomedls::transaction(uint64_t id, string transactionId)
{
    require_auth(_self);

    duels _dl(CONTRACTN, CONTRACTN.value);

    auto dl_itr = _dl.find(id);
    check(dl_itr != _dl.end(), "Duel with id " + to_string(id) + " doesn't exist!");

    _dl.modify(dl_itr, get_self(), [&](auto &mod_duel) {
        mod_duel.transaction = transactionId;
    });
}

void clashdomedls::setelo(name account, uint64_t game, uint64_t value)
{
    require_auth(_self);

    players _pl(CONTRACTN, CONTRACTN.value);

    auto pl_itr = _pl.find(account.value);

    check(pl_itr != _pl.end(), "Account with name " + account.to_string() + " doesn't exist!");

    uint64_t pos = finder(pl_itr->games, game);

    string gameString = "";

    if (game == GameType::ENDLESS_SIEGE) {
        gameString = "Endless Siege";
    } else if (game == GameType::CANDY_FIESTA) {
        gameString = "Candy Fiesta";
    } else {
        gameString = "Non Existing Game";
    }

    check(pos != -1, "Account with name " + account.to_string() + " hasn't played " + gameString);

    _pl.modify(pl_itr, get_self(), [&](auto &mod_player) {
        mod_player.games.at(pos).MMR = value;
    });
}

void clashdomedls::remove(uint64_t id)
{
    require_auth(_self);

    duels _dl(CONTRACTN, CONTRACTN.value);

    auto dl_itr = _dl.find(id);
    check(dl_itr != _dl.end(), "Duel with id " + to_string(id) + " doesn't exist!");
    _dl.erase(dl_itr);
}

void clashdomedls::removeall() {

    require_auth(_self);

    duels _dl(CONTRACTN, CONTRACTN.value);
    for (auto dl_itr = _dl.begin(); dl_itr != _dl.end();) {
        dl_itr = _dl.erase(dl_itr);
    }

    players _pl(CONTRACTN, CONTRACTN.value);
    for (auto pl_itr = _pl.begin(); pl_itr != _pl.end();) {
        pl_itr = _pl.erase(pl_itr);
    }
}

void clashdomedls::logremove(
    uint64_t id
) {
    require_auth(_self);
}

void clashdomedls::transfer(const name &from, const name &to, const asset &quantity, const string &memo)
{
    require_auth(from);

    if (from == _self) {
        return;
    }

    check(EOS_CONTRACT == get_first_receiver(), "invalid contract");
    check(to == _self, "contract is not involved in this transfer");
    check(quantity.symbol.is_valid(), "invalid quantity");
    check(quantity.amount > 0, "only positive quantity allowed");
    check(quantity.symbol == WAX_SYMBOL, "only WAX tokens allowed");

    asset ludio;
    ludio.symbol = LUDIO_SYMBOL;

    ludio.amount = quantity.amount * WAX_TO_LUDIO_RATIO * 0.0001; // decimal conversion LUDIO 4 decimals, WAX 8 decimals

    action(permission_level{_self, "active"_n}, LUDIO_CONTRACT, "transfer"_n, make_tuple(_self, from, ludio, string("Duel participation Ludio reward."))).send(); 

    payments _py(CONTRACTN, CONTRACTN.value);

    uint64_t game = -1;

    if (memo.find("Endless Siege") != string::npos) {
        game = GameType::ENDLESS_SIEGE;
    } else if (memo.find("Candy Fiesta") != string::npos) {
        game = GameType::CANDY_FIESTA;
    }

    // TODO: enable this
    check(game != -1, "Invalid payment memo.");

    auto py_itr = _py.find(from.value);

    // new user
    if (py_itr == _py.end()) {

        vector<asset> quantities;
        quantities.push_back(quantity);

        vector<payment_info> games;
        games.push_back({ game, quantities});

        _py.emplace(CONTRACTN, [&](auto& new_player) {
            new_player.account = from;
            new_player.games = games;
        });
    } else {

        uint64_t pos = finder(py_itr->games, game);

        // new game
        if (pos == -1) {

            vector<asset> quantities;
            quantities.push_back(quantity);

            _py.modify(py_itr, get_self(), [&](auto &mod_player) {
                mod_player.games.push_back({game, quantities});
            });

        } else {

            _py.modify(py_itr, get_self(), [&](auto &mod_player) {
                mod_player.games.at(pos).quantities.push_back(quantity);
            });
        }
    }
}

uint64_t clashdomedls::finder(vector<game_info> games, uint64_t id)
{
    for (uint64_t i = 0; i < games.size(); i++)
    {
        if (games.at(i).id == id)
        {
            return i;
        }
    }
    return -1;
}

uint64_t clashdomedls::finder(vector<payment_info> games, uint64_t id)
{
    for (uint64_t i = 0; i < games.size(); i++)
    {
        if (games.at(i).id == id)
        {
            return i;
        }
    }
    return -1;
}

void clashdomedls::checkPayments(uint64_t game, asset fee, name account)
{

    payments _py(CONTRACTN, CONTRACTN.value);

    auto py_itr = _py.find(account.value);

    check(py_itr != _py.end(), "Entry fee not found.");

    uint64_t pos = finder(py_itr->games, game);

    check(pos != -1, "Entry fee not found.");

    uint64_t count = -1;

    for (uint64_t i = 0; i < py_itr->games.at(pos).quantities.size(); i++)
    {
        if (py_itr->games.at(pos).quantities.at(i) == fee)
        {
            count = i;
            break;
        }
    }

    check(count != -1, "Entry fee not found.");

    _py.modify(py_itr, get_self(), [&](auto &mod_player) {
        mod_player.games.at(pos).quantities.erase(mod_player.games.at(pos).quantities.begin() + count);
    });
}