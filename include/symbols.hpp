#ifndef SYMBOLS_HPP
#define SYMBOLS_HPP

#include <string>
#include <elf/elf++.hh>

enum class symbol_type {
    notype,            // No type (e.g., absolute symbol)
    object,            // Data object
    func,              // Function entry point
    section,           // Symbol is associated with a section
    file,              // Source file associated with the
};                     // object file

std::string to_string (symbol_type st) {
    switch (st) {
    case symbol_type::notype: return "notype";
    case symbol_type::object: return "object";
    case symbol_type::func: return "func";
    case symbol_type::section: return "section";
    case symbol_type::file: return "file";
    default: return "";
    }
}

symbol_type to_symbol_type(elf::stt sym) {
    switch (sym) {
    case elf::stt::notype: return symbol_type::notype;
    case elf::stt::object: return symbol_type::object;
    case elf::stt::func: return symbol_type::func;
    case elf::stt::section: return symbol_type::section;
    case elf::stt::file: return symbol_type::file;
    default: return symbol_type::notype;
    }
};

struct symbol {
    symbol_type type;
    std::string name;
    std::uintptr_t addr;
};

#endif