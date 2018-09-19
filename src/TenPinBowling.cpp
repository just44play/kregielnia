#include "TenPinBowling.hpp"
#include <stdexcept>
#include <regex>

TenPinBowling::TenPinBowling(std::string const & name, std::vector<Game> const & game,
                             fs::path const & input_dir_path, fs::path const & output_file_path)
    :name_(name)
    ,games_(game)
    ,input_dir_path_(input_dir_path)
    ,output_file_path_(output_file_path)
{}

TenPinBowling::~TenPinBowling()
{}

bool TenPinBowling::loadInputFiles()
{
    std::vector<Player> game{};

    if (not fs::is_directory(input_dir_path_) or fs::is_empty(input_dir_path_)) {
        throw std::logic_error("There is lack of input files!");
    }

    std::vector<std::string> filePaths;
    for (auto & p : fs::directory_iterator(input_dir_path_)) {

        filePaths.push_back(p.path());
    }
    std::sort(filePaths.begin(), filePaths.end());

    for(auto & filePath: filePaths) {
        std::ifstream fileStream(filePath);
        for (std::string singleLine; std::getline(fileStream, singleLine); ) {
            if(isValidPlayer(singleLine)) {
                auto singlePlayer = std::make_tuple(getPlayerName(singleLine), getPlayerFrame(singleLine),
                                                    calculateScore(getPlayerFrame(singleLine)), getPlayerStatus(singleLine));
                game.push_back(singlePlayer);
            } else {
                throw std::logic_error("Error: Invalid InputFile!");
            }
        }
        games_.push_back(game);
        game.clear();
    }
    return true;
}

int TenPinBowling::gamesCounter()
{
    return games_.size();
}

bool TenPinBowling::isValidPlayer(std::string const & str)
{
    std::regex pattern("(\\w*)(:)(((([\\dX/-]{1,2})([|])?){0,10})|((([\\dX/-]{1,2})([|])?){10}([||]([\\dX/-]{1,2})?)?))");
    return std::regex_match(str, pattern);
}

int lambda(char c)
{
    if(c == '-'){
        return 0;
    }else if(std::isdigit(c) ){
        return c - 48;
    }else if(c == 'X') {
        return 10;
    }
    return 0;
}

int SumLastTwoThrows(std::string const & playerFrames)
{
    return lambda(*playerFrames.rbegin()) + lambda(*playerFrames.rbegin()+1);
}

int CountFramesInGame(std::string const & playerFrames)
{
    return std::count(std::begin(playerFrames), std::end(playerFrames), '|');
}

int CountThrowsInLastFrame(std::string const & playerFrames)
{
    return std::distance(std::find(std::rbegin(playerFrames), std::rend(playerFrames), '|'), std::rbegin(playerFrames));
}

char CompliteSpare(char firstThrow)
{
    return *std::to_string( 10 - (static_cast<int>(firstThrow) - 48) ).c_str();
}

std::string ExtractPointsFromString(std::string const & playerFrames)
{
    std::string str = playerFrames;
    auto const pos=playerFrames.find_last_of(':');
    if(pos != std::string::npos)
        str.erase(str.begin(), str.begin()+pos+1);

    str.erase(std::remove(str.begin(), str.end(), '|'),str.end());

    for(int i = 0; i < str.length(); i ++)
        if(str[i] == '/')
            str[i] = CompliteSpare(str[i-1]);
    return str;
}

std::vector<int> TenPinBowling::getPlayerFrame(std::string const & playerFrames)
{
    std::string str = ExtractPointsFromString(playerFrames);

    std::vector<int> vec;

    std::transform(str.begin(), str.end(),
                   std::back_inserter(vec),
                   [vec](char c) {return lambda(c);});
    return vec;
}

std::string TenPinBowling::getPlayerName(std::string const &playerFrames)
{

    auto const pos=playerFrames.find_last_of(':');

    std::string name;
    std::copy(playerFrames.begin(),
              playerFrames.begin() + pos,
              std::back_inserter(name));
    return name;
}

Status TenPinBowling::getPlayerStatus(std::string const & playerFrames)
{
    if(playerFrames.length() > 0){
        if(CountFramesInGame(playerFrames) == 9 && CountThrowsInLastFrame(playerFrames) == -2 && SumLastTwoThrows(playerFrames) < 10)
                return Status::Finished;

        if(CountFramesInGame(playerFrames) == 11 &&
            ((CountThrowsInLastFrame(playerFrames) == -1 && *playerFrames.rbegin() != 'X') ||
                    CountThrowsInLastFrame(playerFrames) == -2))
                return Status::Finished;

        return Status::InProgress;
    }

    return Status::NotStarted;
}

int TenPinBowling::calculateScore(const std::vector<int> &vec)
{
    if (vec.size() == 1)
        return vec[0];
    int sum=0;
    int frames = 1;
    for (int i = 0; i < vec.size(); i++) {
        if(vec[i] == 10) {
            if (i == vec.size()-1 && frames <= 10) {
                sum += vec[i];
            }
            else if (i == vec.size() - 2 && frames <= 10) {
                sum += vec[i] + vec[i+1];
            }
            else if (i < vec.size() - 2) {
                sum += vec[i] + vec[i+1] + vec[i+2];
            }
            frames++;
        }
        else if (i<vec.size()-2 && vec[i]+vec[i+1] == 10 && frames<=10) {
            sum += vec[i] + vec[i+1] + vec[i+2];
            i++;
            frames++;
        }
        else if (i<vec.size()-2 && frames<=10) {
            sum += vec[i] + vec[i+1];
            i++;
            frames++;
        }
        else if (frames <= 10) {
            sum+=vec[i];
        }
    }
    return sum;
}

std::string getLaneStatus(Game& game)
{
    if (std::all_of(game.begin(), game.end(),
        [&](Player& p){ return std::get<3>(p) == Status::NotStarted; })) {

            return "no game";
    }
    else if (std::all_of(game.begin(), game.end(),
        [&](Player& p){ return std::get<3>(p) == Status::Finished; })) {

            return "game finished";
    }
    else return "game in progress";
}

void TenPinBowling::outputResults(bool isPrintOnConsoleRequest)
{
    if (isPrintOnConsoleRequest == true) {
        for (int i = 0; i < games_.size(); i++) {
            std::cout <<"## " << "Lane " << i+1 << ": "
                      << getLaneStatus(games_[i])
                      << " ##" << std::endl;
            for (int p = 0; p < games_[i].size(); p++) {
                std::cout << std::get<0>(games_[i][p]) << " "
                          << std::get<2>(games_[i][p]) << std::endl;
            }
        }
    }
    else {
        std::fstream file;
        file.open(output_file_path_, std::ios::out);

        for (int i = 0; i < games_.size(); i++) {
            file << "## " << "Lane " << i+1 << ": "
                      << getLaneStatus(games_[i])
                      << " ##" << std::endl;
            for (int p = 0; p < games_[i].size(); p++) {
                file << std::get<0>(games_[i][p]) << " "
                          << std::get<2>(games_[i][p]) << std::endl;
            }
        }

        file.close();
    }
}
