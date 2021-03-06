#include "../dwarf.hh"

#include <iostream>
#include <sys/stat.h>
#include <assert.h>
#include <cstdio>

void Dwarf::gcov(std::uint64_t rip)
{
    for (auto& elt : m_list_range)
        if (rip >= elt.begin && rip < (elt.begin + elt.offset))
        {
            gcov_incr_line_count(rip, elt);
            return;
        }
}

void Dwarf::gcov_incr_line_count(std::uint64_t rip,
        struct range_addr& range_addr)
{
    static unsigned int old_line_number = 0;
    reset_registers();

    if (!get_line_number(rip, range_addr.debug_line_offset))
    {
        std::cerr << "Can't find the line corresponding to this instruction.";
        std::cerr << std::endl;
        std::exit(1);
    }

    if (m_reg_line == old_line_number)
        return;

    else
        old_line_number = m_reg_line;

    unsigned char* comp_dir = &m_buf[range_addr.comp_dir_offset];
    unsigned char* file_name = &m_buf[range_addr.file_name_offset];

    std::string file_path(reinterpret_cast<char*>(comp_dir));
    std::string file_name_string(reinterpret_cast<char*>(file_name));

    if (file_name_string[0] == '/')
        file_path = file_name_string;

    else
        file_path += "/" + file_name_string;

    if (m_reg_line - 1 < m_gcov_vect[file_path].size())
        ++m_gcov_vect[file_path][m_reg_line - 1];
}

void Dwarf::write_result_gcov()
{
    for (auto& elt : m_gcov_vect)
    {
        std::shared_ptr<std::ifstream> file_ptr = m_map_ifstream[elt.first];
        file_ptr->seekg(file_ptr->beg);

        std::string line;

        const char* file_path = elt.first.c_str();
        std::size_t i;

        assert(elt.first.length() > 0);
        for (i = elt.first.length() - 1; i > 0
                && file_path[i] != '/'; --i)
            ;

        std::string output_file_string(&file_path[i ? i + 1 : i]);
        mkdir("cov_files", 0755);
        output_file_string = "cov_files/" + output_file_string + ".cov";

        FILE* output_file = std::fopen(output_file_string.c_str(), "w");

        for (i = 1; std::getline(*file_ptr, line); ++i)
        {
            if (elt.second[i - 1] == 0)
                std::fprintf(output_file, "-:\t");

            else
                std::fprintf(output_file, "%d:\t", elt.second[i - 1]);

            std::fprintf(output_file, "%.4lu:", i);
            std::fprintf(output_file, "%s\n", line.c_str());
        }

        std::fputc('\n', output_file);
        std::fclose(output_file);
    }
}
