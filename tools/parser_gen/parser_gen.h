// LICENSE
// This software is free for use and redistribution while including this
// license notice, unless:
// 1. is used for commercial or non-personal purposes, or
// 2. used for a product which includes or associated with a blockchain or other
// decentralized database technology, or
// 3. used for a product which includes or associated with the issuance or use
// of cryptographic or electronic currencies/coins/tokens.
// On all of the mentioned cases, an explicit and written permission is required
// from the Author (Ohad Asor).
// Contact ohad@idni.org for requesting a permission. This license may be
// modified over time by the Author.
#ifndef __IDNI__PARSER__PARSER_GEN_H__
#define __IDNI__PARSER__PARSER_GEN_H__
#include "parser.h"
namespace idni {

template <typename C = char, typename T = C>
struct grammar_inspector { // provides access to private grammar members
	grammar_inspector(const grammar<C, T>& g) : g(g) {}
	const grammar<C, T>& g;
	lit<C, T> start() { return g.start; }
	nonterminals<C, T>& nts() { return g.nts; }
	const std::vector<std::pair<lit<C, T>, std::vector<lits<C, T>>>>& G(){
		return g.G; }
	const char_class_fns<C>& cc_fns() { return g.cc_fns; }
};

template <typename C = char> std::string char_type();
template<> std::string char_type<char>()     { return "char"; }
template<> std::string char_type<char32_t>() { return "char32_t"; }

template <typename C = char, typename T = C>
std::ostream& generate_parser_cpp(std::ostream& os, const std::string& name, const std::string tgf_filename) {
	static_assert(std::is_same_v<C, char> || std::is_same_v<T, char32_t>);
	static_assert(std::is_same_v<C, T>);
	nonterminals<C, T> nts;
	auto g = tgf<C, T>::from_file(nts, name, tgf_filename);
	if (g.size() == 0) return os;
	const std::string cht = char_type<C>();
	std::string U = cht == "char32_t" ? "U" : "";
	std::vector<C> ts{ (C)0 };
	grammar_inspector<C, T> gi(g);
	auto gen_ts = [&ts, &U]() {
		std::stringstream os;
		os << "\t\t";
		size_t n = 0;
		for (const C ch : ts) {
			if (++n == 10) n = 0, os << "\n\t\t";
			os << U <<"'" <<
				( ch == (C)    0 ? "\\0"
				: ch == (C) '\r' ? "\\r"
				: ch == (C) '\n' ? "\\n"
				: ch == (C) '\t' ? "\\t"
				: ch == (C) '\\' ? "\\\\"
				: ch == (C) '\'' ? "\\'"
				: to_std_string(ch)) << "', ";
		}
		os << "\n";
		return os.str();
	};
	auto gen_nts_enum_cte = [&gi, &U]() {
		std::stringstream os;
		auto& x = gi.nts();
		for (size_t n = 0; n != x.size(); ++n)
			os << (n % 10 == 0 ? "\n\t\t\t" : "") <<
				U << to_std_string(x[n]) << ", ";
		os << "\n";
		return os.str();
	};
	auto gen_nts = [&gi, &U]() {
		std::stringstream os;
		auto& x = gi.nts();
		for (size_t n = 0; n != x.size(); ++n)
			os << (n % 10 == 0 ? "\n\t\t\t" : "") <<
				U << "\"" << to_std_string(x[n]) << "\", ";
		os << "\n";
		return os.str();
	};
	auto gen_cc_fns = [&gi, &U]() {
		std::stringstream os;
		auto& x = gi.nts();
		for (const auto& fn : gi.cc_fns().fns)
			if (x[fn.first].size()) os<<"\t\t\t\""
				<< to_std_string(x[fn.first]) << "\",\n";
		return os.str();
	};
	auto gen_prods = [&gi, &ts]() {
		std::stringstream os;
		auto terminal = [&ts](const lit<C, T>& l) {
			for (size_t n = 0; n != ts.size(); ++n)
				if (ts[n] == l.t()) return n;
			ts.push_back(l.t());
			return ts.size()-1;
		};
		for (const auto& p : gi.G()) {
			os << "\t\tq(nt(" << p.first.n() << "), ";
			bool first_c = true;
			for (const auto& c : p.second) {
				if (!first_c) os << " & "; else first_c = false;
				os << '(';
				bool first_l = true;
				for (const auto& l : c) {
					if (!first_l) os << "+";
					else first_l = false;
					if (l.nt()) os << "nt(" << l.n() << ")";
					else if (l.is_null()) os << "nul";
					else os << "t(" << terminal(l) << ")";
				}
				os << ')';
			}
			os << ");\n";
		}
		return os.str();
	};
	auto comment_grammar = [&tgf_filename, &name] {
		// print the file whose filename is name line by line
		std::stringstream os;
		std::ifstream f(name);
		std::string line;
		while (std::getline(f, line)) os << "// " << line << "\n";
		return os.str();
	};
	const auto ps = gen_prods();
	os <<	"// This file is generated by \n"
		"//       https://github.com/IDNI/parser/tools/parser_gen\n"
		"// from the grammar in file: " << name << "\n"
		"// The content of the file " << name << " is:\n" << comment_grammar() << "\n" <<
		"#include <string.h>\n"
		"#include \"parser.h\"\n"
		"struct " << name << " {\n"
		"	" << name << "() :\n"
		"		nts(load_nonterminals()), cc(load_cc()),\n"
		"		g(nts, load_prods(), nt(" << gi.start().n() <<
					"), cc), p(g) { }\n"
		"	std::unique_ptr<typename idni::parser<" <<cht<< ">"
							"::pforest> parse(\n"
		"		const " <<cht<< "* data, size_t size = 0,\n"
		"		" <<cht<< " eof = std::char_traits<" <<cht<< ">"
								"::eof())\n"
		"			{ return p.parse(data, size, eof); }\n"
		"	std::unique_ptr<typename idni::parser<" <<cht<< ">"
							"::pforest> parse(\n"
		"		std::basic_istream<" <<cht<< ">& is,\n"
		"		size_t size = 0,\n"
		"		" <<cht<< " eof = std::char_traits<" <<cht<< ">"
								"::eof())\n"
		"			{ return p.parse(is, size, eof); }\n"
		"	bool found() { return p.found(); }\n"
		"	typename idni::parser<" <<cht<< ">::perror_t "
								"get_error()\n"
		"		{ return p.get_error(); }\n"
		"   enum struct nonterminal : size_t {\n" << gen_nts_enum_cte() << "   };\n"
		"private:\n"
		"	std::vector<" <<cht<< "> ts{\n" <<
				gen_ts() <<
		"	};\n"
		"	idni::nonterminals<" <<cht<< "> nts{};\n"
		"	idni::char_class_fns<" <<cht<< "> cc;\n"
		"	idni::grammar<" <<cht<< "> g;\n"
		"	idni::parser<" <<cht<< "> p;\n"
		"	idni::prods<" <<cht<< "> t(size_t tid) {\n"
		"		return idni::prods<" <<cht<< ">(ts[tid]);\n"
		"	}\n"
		"	idni::prods<" <<cht<<"> nt(size_t ntid) {\n"
		"		return idni::prods<"<<cht<< ">("
					"idni::lit<"<<cht<<">(ntid, &nts));\n"
		"	}\n"
		"	idni::nonterminals<" <<cht<< "> load_nonterminals() "
								"const {\n"
		"		idni::nonterminals<" <<cht<< "> nts{};\n"
		"		for (const auto& nt : {" <<
					gen_nts() <<
		"		}) nts.get(nt);\n"
		"		return nts;\n"
		"	}\n"
		"	idni::char_class_fns<" <<cht<< "> load_cc() {\n"
		"		return idni::predefined_char_classes<" <<cht<<
								">({\n" <<
					gen_cc_fns() <<
		"		}, nts);\n"
		"	}\n"
		"	idni::prods<" <<cht<< "> load_prods() {\n"
		"		idni::prods<" <<cht<< "> q, nul(idni::lit<" <<
					cht<< ">{});\n" << ps <<
		"		return q;\n"
		"	}\n"
		"};\n";
	return os;
}

template <typename C = char, typename T = C>
std::ostream& generate_parser_cpp_from_string(std::ostream& os,
	const std::string& name,
	const std::basic_string<C>& grammar_tgf,
	const std::basic_string<C>& start_nt = from_cstr<C>("start"))
{
	nonterminals<C, T> nts;
	return generate_parser_cpp(os, name,
		tgf<C, T>::from_string(nts, grammar_tgf, start_nt), grammar_tgf);
}

template <typename C = char, typename T = C>
void generate_parser_cpp_from_file(std::ostream& os, const std::string& name,
	const std::string& tgf_filename,
	const std::basic_string<C>& start_nt = from_cstr<C>("start"))
{
	nonterminals<C, T> nts;
	generate_parser_cpp(os, name, tgf_filename);
}

} // idni namespace
#endif //__IDNI__PARSER__PARSER_GEN_H__
