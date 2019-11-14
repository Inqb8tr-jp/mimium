#pragma once

#include <memory>
#include <vector>
#include <string>
#include <variant>
#include <unordered_map>

namespace mimium{
    //https://medium.com/@dennis.luxen/breaking-circular-dependencies-in-recursive-union-types-with-c-17-the-curious-case-of-4ab00cfda10d
    template <typename T> struct recursive_wrapper {
    // construct from an existing object
    recursive_wrapper(T t_) { t.emplace_back(std::move(t_)); }
    // cast back to wrapped type
    operator const T &() const { return t.front(); }
    // store the value
    std::vector<T> t;
    std::string toString();
    };

    namespace types{
        struct Void{
            std::string toString(){
                return "Void";
            }
        };
        struct Float{
            std::string toString(){
                return "Float";
            };
        };
        struct String{
            std::string toString(){
                return "String";
            };
        };
        struct Function;
        struct Array;
        struct Time;
        using Value = std::variant<types::Void,types::Float,types::String,recursive_wrapper<types::Function>,recursive_wrapper<types::Array>,recursive_wrapper<types::Time>>;
        struct Function {
            Function(){
            }
            Function(std::vector<Value> _arg_types,Value _ret_type): arg_types(std::move(_arg_types)),ret_type(std::move(_ret_type)){};
            void init(std::vector<Value> _arg_types,Value _ret_type){
                arg_types = std::move(_arg_types);
                ret_type = std::move(_ret_type);
            }
            std::vector<Value> arg_types;
            Value ret_type;
            Value& getReturnType(){
                return ret_type;
            }
            std::vector<Value>& getArgTypes(){
                return arg_types;
            }
            std::string toString(){
                std::string s = "(";
                int count =0;
                for(const auto& v: arg_types){
                    s+= std::visit([](auto c){return c.toString();},v);
                    if(count < arg_types.size()) s+= " , ";
                    count++;
                }
                s+=")->" + std::visit([](auto c){return c.toString();},ret_type);
                return s;
            };
        };
        struct Array{
            Value elem_type;
            Array(Value elem):elem_type(elem){

            }
            std::string toString(){
                return "array["+ std::visit([](auto c){return c.toString();},elem_type)+ "]";
            }
            Value getElemType(){
                return elem_type;
            }
        };
        struct Time{
            Float val;
            Float time;
            Time(){
            } 
            std::string toString(){
                return val.toString() + "@" +time.toString(); 
            }

        };
    }//namespace types
    class TypeEnv{
        public:
        std::unordered_map<std::string,types::Value> env;

    };
}//namespace mimium