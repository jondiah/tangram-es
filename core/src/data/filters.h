#pragma once

#include "tileData.h"
#include <unordered_map>
#include <vector>

namespace Tangram {

    struct Value {

        std::string str;
        float num;
        bool numeric;

        bool equals(float f) const { return numeric && num == f; }
        bool equals(const std::string& s) const { return str.size() != 0 && str == s; }
        bool equals(const Value& v) const { return (numeric && v.equals(num)) || v.equals(str); }

        Value() : num(0), numeric(false) {}
        Value(float n) : num(n), numeric(true) {}
        Value(float n, const std::string& s) : str(s), num(n), numeric(true) {}
        Value(const std::string& s) : str(s), num(0), numeric(false) {}

        // Why do "numeric" values keep both a string and a number? Basically because of a shortcoming in the
        // YAML parser we use. Suppose we want to filter for features named "007". In a stylesheet, filter values 
        // can be either numbers or strings and the only way to check for numbers is to try to cast the value to 
        // a numeric type. The cast succeeds for "007", so we must consider it a number value. But when we filter
        // against a feature containing the string "007", we must also have the original string representation of
        // the filter value in order to correctly find the match. 

    };

    using Context = std::unordered_map<std::string, Value>;

    enum class Operators : int {
        any = 0,
        all,
        none
    };
    
    enum class FilterType : int {
        any = 0,
        all,
        none,
        existence,
        equality,
        range
    };
    
    struct Filter {
        
        std::vector<Filter> operands;
        std::vector<Value> values;
        std::string key;
        bool exists;
        
        FilterType type;
        
        Filter() : type(FilterType::none) {}
        
        // Create an 'any', 'all', or 'none' filter
        Filter(Operators op, const std::vector<Filter>& filters) : operands(filters), type(static_cast<FilterType>(op)) {}
        
        // Create an 'equality' filter
        Filter(const std::string& k, const std::vector<Value>& vals) : values(vals), key(k), type(FilterType::equality) {}
        
        // Create a 'range' filter
        Filter(const std::string& k, float min, float max) : values({ Value(min), Value(max) }), key(k), type(FilterType::range) {}
        
        // Create an 'existence' filter
        Filter(const std::string& k, bool ex) : key(k), exists(ex), type(FilterType::existence) {}

        bool eval(const Feature& feat, const Context& ctx) const {
            
            switch (type) {
                    
                case FilterType::any:
                case FilterType::all:
                case FilterType::none: {
                    
                    bool breakValue = (type == FilterType::any || type == FilterType::none); // For 'any' or 'none' filters, a true evaluation breaks early
                    bool breakResult = (type == FilterType::any); // For 'any' filters, an early break returns true
                    for (const auto& filt : operands) {
                        if (filt.eval(feat, ctx) == breakValue) { return breakResult; }
                    }
                    return !breakResult;
                }
                case FilterType::existence: {
                    
                    bool found = ctx.find(key) != ctx.end() ||
                    feat.props.stringProps.find(key) != feat.props.stringProps.end() ||
                    feat.props.numericProps.find(key) != feat.props.numericProps.end();
                    
                    return exists == found;
                }
                case FilterType::equality: {
                    
                    auto ctxIt = ctx.find(key);
                    if (ctxIt != ctx.end()) {
                        for (auto& v : values) {
                            if (v.equals(ctxIt->second)) { return true; }
                        }
                        return false;
                    }
                    auto strIt = feat.props.stringProps.find(key);
                    if (strIt != feat.props.stringProps.end()) {
                        for (auto& v : values) {
                            if (v.equals(strIt->second)) { return true; }
                        }
                    }
                    auto numIt = feat.props.numericProps.find(key);
                    if (numIt != feat.props.numericProps.end()) {
                        for (auto& v : values) {
                            if (v.equals(numIt->second)) { return true; }
                        }
                    }
                    return false;
                }
                case FilterType::range: {
                    
                    float min = values[0].num;
                    float max = values[1].num;
                    auto ctxIt = ctx.find(key);
                    if (ctxIt != ctx.end()) {
                        const auto& val = ctxIt->second;
                        if (!val.numeric) { return false; } // only check range for numbers
                        return val.num >= min && val.num < max;
                    }
                    auto numIt = feat.props.numericProps.find(key);
                    if (numIt != feat.props.numericProps.end()) {
                        const auto& num = numIt->second;
                        return num >= min && num < max;
                    }
                    return false;
                }
                default:
                    return true;
            }
        }

    };
}
