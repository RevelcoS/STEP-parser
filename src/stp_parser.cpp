#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <utility>
#include <regex>
#include <fstream>
#include <sstream>
#include <cassert>

#include <LiteMath.h>

#include "clstepcore/ExpDict.h"
#include "cleditor/STEPfile.h"
#include "clstepcore/STEPattribute.h"
#include "clstepcore/sdai.h"

#include <stp_parser.hpp>

using namespace LiteMath;
using regexiter_t = std::sregex_token_iterator;

extern void SchemaInit( Registry & );

namespace STEP {

Type str2type(std::string name) {
    if (name == "B_SPLINE_SURFACE_WITH_KNOTS")
        return Type::BSPLINE_SURFACE;
    else if (name == "CARTESIAN_POINT")
        return Type::POINT;
    return Type::UNDEFINED;
}

std::vector<std::string> argsplit(const std::string &rawargs) {
    int level = 0;
    std::vector<uint> argpos = { 1 };
    for (uint idx = 0; idx < rawargs.size(); idx++) {
        char chr = rawargs[idx];
        if (chr == '(') level++;
        else if (chr == ')') level--;
        else if (chr == ',' && level == 1) argpos.push_back(idx + 1);
    }
    argpos.push_back(rawargs.size());

    std::vector<std::string> args;
    for (uint idx = 1; idx < argpos.size(); idx++) {
        uint cur = argpos[idx - 1], next = argpos[idx];
        std::string arg = rawargs.substr(cur, next - cur - 1);
        args.push_back(arg);
    }

    return args;
}

uint Parser::parseID(std::string rawID) {
    std::regex rexp("#(\\d+)");
    std::smatch match;
    std::regex_match(rawID, match, rexp);
    uint id = std::stoi(match[1].str());
    return id;
}

uint Parser::parseU(std::string raw) {
    return std::stoi(raw);
}

std::vector<uint> Parser::parseUVector1D(std::string raw) {
    std::vector<std::string> args = argsplit(raw);
    std::vector<uint> uvector1D;
    for (auto arg : args) {
        uint num = stoi(arg);
        uvector1D.push_back(num);
    }
    return uvector1D;
}

std::vector<float> Parser::parseFVector1D(std::string raw) {
    std::vector<std::string> args = argsplit(raw);
    std::vector<float> fvector1D;
    for (auto arg : args) {
        float num = stof(arg);
        fvector1D.push_back(num);
    }
    return fvector1D;
}

float3 Parser::tofloat3(uint id) {
    auto &entity = this->entities[id];
    assert(entity.type == Type::POINT);

    float3 point = float3();
    std::string coords_arg = entity.args[1];

    // Parse coords
    std::vector<std::string> coords = argsplit(coords_arg);
    assert(coords.size() == 3);
    for (size_t i = 0; i < coords.size(); i++) {
        point[i] = std::stof(coords[i]);
    }

    return point;
}

Vector2D<float4> Parser::parsePointVector2D(std::string raw) {
    std::vector<std::string> points_rows = argsplit(raw);
    size_t rows = points_rows.size();
    size_t cols = argsplit(points_rows[0]).size();
    Vector2D<float4> points(rows, cols);
    for (size_t i = 0; i < rows; i++) {
        std::vector<std::string> points_row = argsplit(points_rows[i]);
        for (size_t j = 0; j < cols; j++) {
            std::string rawID = points_row[j];
            uint pointID = this->parseID(rawID);
            float3 point = this->tofloat3(pointID);
            auto index = std::make_pair(i, j);
            points[index] = ::to_float4(point, 1.0f);
        }
    }
    return points;
}

bool Parser::tryParseEntity(const std::string &entry, Entity &res) {
    std::regex rexp;
    std::smatch matches;
    uint id;
    Type type;
    std::vector<std::string> args;

    // Match the id of entity
    rexp = std::regex("#(\\d+)=");
    if (!std::regex_search(entry, matches, rexp))
      return false;

    id = std::stoi(matches[1].str());

    // Context or settings?
    rexp = std::regex("=\\(");
    if (std::regex_search(entry, matches, rexp))
        return false;

    // Match the type of entity
    rexp = std::regex("=(\\S+?)\\(");
    std::regex_search(entry, matches, rexp);

    type = str2type(matches[1].str());

    // Match the arguments of entity
    rexp = std::regex("=\\S+?(\\(.+\\))");
    std::regex_search(entry, matches, rexp);

    std::string rawargs = matches[1].str(); 
    args = argsplit(rawargs);
    
    res.id = id;
    res.type = type;
    res.args = args;
    return true;
} 

std::vector<float> decompressKnots(
        std::vector<float> &knots_comp,
        std::vector<uint> &knots_mult) {
    assert(knots_comp.size() == knots_mult.size());

    std::vector<float> knots;
    for (size_t i = 0; i < knots_comp.size(); i++) {
        for (size_t j = 0; j < knots_mult[i]; j++) {
            knots.push_back(knots_comp[i]);
        }
    }
    return knots;
}

void trimKnots(std::vector<float> &knots, const std::vector<uint> &knots_mult, uint degree) {
    // Convert first and last knots to 'nurbs' format,
    // so that they are repated p+1 times where p is corresponding degree.
    // This is a crutch, because there should be an option in STEP
    // that corresponds to the degree multiplicities type.
    if (knots_mult[0] == 1) {
        for (size_t i = 0; i < degree; i++) {
            knots[i] = knots[degree];
        }
    }

    size_t size = knots.size();
    if (knots_mult[size-1] == 1) {
         for (size_t i = 0; i < degree; i++) {
            knots[size-i-1] = knots[size-degree-1];
         }
    }
}

RawNURBS Parser::toNURBS(uint id) {
    Entity &entity = this->entities[id];
    assert(entity.type == Type::BSPLINE_SURFACE);

    RawNURBS nurbs;

    std::string u_degree_arg     = entity.args[1];
    std::string v_degree_arg     = entity.args[2];
    std::string points_arg       = entity.args[3];
    std::string u_knots_mult_arg = entity.args[8];
    std::string v_knots_mult_arg = entity.args[9];
    std::string u_knots_arg      = entity.args[10];
    std::string v_knots_arg      = entity.args[11];

    // Parse degrees
    nurbs.u_degree = this->parseU(u_degree_arg);
    nurbs.v_degree = this->parseU(v_degree_arg);

    // Parse control points
    nurbs.points = this->parsePointVector2D(points_arg);

    // Parse knot_multiplicities
    std::vector<uint> u_knots_mult = this->parseUVector1D(u_knots_mult_arg);
    std::vector<uint> v_knots_mult = this->parseUVector1D(v_knots_mult_arg);

    // Parse knots
    std::vector<float> u_knots_comp = this->parseFVector1D(u_knots_arg);
    std::vector<float> v_knots_comp = this->parseFVector1D(v_knots_arg);
    nurbs.u_knots = decompressKnots(u_knots_comp, u_knots_mult);
    nurbs.v_knots = decompressKnots(v_knots_comp, v_knots_mult);
 
    // Trim knots
    trimKnots(nurbs.u_knots, u_knots_mult, nurbs.u_degree);
    trimKnots(nurbs.v_knots, v_knots_mult, nurbs.v_degree);

    // Make default weights
    size_t rows = nurbs.points.rows_count(), cols = nurbs.points.cols_count();
    nurbs.weights = Vector2D<float>(rows, cols);
    for (size_t i = 0; i < rows; i++)
        for (size_t j = 0; j < cols; j++) {
            auto index = std::make_pair(i, j);
            nurbs.weights[index] = 1;
        }

    return std::move(nurbs);
}

std::vector<RawNURBS> Parser::allNURBS() {
    std::vector<RawNURBS> allNurbs;
    for (auto &pair : this->entities) {
        auto id = pair.first;
        auto entity = pair.second;
        if (entity.type == Type::BSPLINE_SURFACE) {
          allNurbs.push_back(this->toNURBS(id));
        }
    }
    return allNurbs;
}

std::map<uint, RawNURBS> Parser::allIDNurbs() {
    std::map<uint, RawNURBS> IDNurbs;
    for (auto &pair : this->entities) {
        auto id = pair.first;
        auto entity = pair.second;
        if (entity.type == Type::BSPLINE_SURFACE) {
          IDNurbs[id] = this->toNURBS(id);
        }
    }
    return IDNurbs;
}

Parser::Parser(const std::string &filename) {
    std::ifstream file(filename);
    std::stringstream stream;
    stream << file.rdbuf();
    std::string text = stream.str();
    file.close();

    // Remove space symbols
    std::regex rexp("\\s+");
    text = std::regex_replace(text, rexp, "");

    // Parse each entry, separated by semicolon
    rexp = std::regex(";");
    regexiter_t it(text.begin(), text.end(), rexp, -1);
    regexiter_t end;

    for (; it != end; it++) {
        Entity entity;
        if (this->tryParseEntity(*it, entity)) 
          this->entities[entity.id] = std::move(entity);
    }
}

std::ostream& operator<<(std::ostream& cout, const STEP::RawNURBS &nurbs) {
    // Control points dimensions
    uint32_t n = nurbs.points.rows_count();
    cout << "n = " << n-1 << std::endl;

    uint32_t m = nurbs.points.cols_count();
    cout << "m = " << m-1 << std::endl;

    // Control points
    cout << "points:" << std::endl;
    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < m; j++) {
            auto index = std::make_pair(i, j);
            auto point = nurbs.points[index];
            cout << "{" << point.x << " " << point.z << " " << point.y << "}\t";
        }
        cout << std::endl;
    }

    // Weights
    cout << "weights:" << std::endl;
    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < m; j++) {
            auto index = std::make_pair(i, j);
            auto point = nurbs.points[index];
            cout << nurbs.weights[index] << " ";
        }
        cout << std::endl;
    }

    // Degrees
    cout << "u_degree: " << nurbs.u_degree << std::endl;
    cout << "v_degree: " << nurbs.v_degree << std::endl;

    // Knots
    cout << "u_knots: ";
    for (auto knot : nurbs.u_knots)
        cout << knot << " ";
    cout << std::endl;

    cout << "v_knots: ";
    for (auto knot : nurbs.v_knots)
        cout << knot << " ";
    cout << std::endl;

    return cout;
}

} // namespace STEP
