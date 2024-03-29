#include "../core/File.hpp"
#include "../core/NeuralNetwork.hpp"
#include <boost/program_options.hpp>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <ostream>
#include <string>

namespace po = boost::program_options;

int main(int argc, char **argv)
{
	po::variables_map options;
	po::options_description cli("Options");
	cli.add_options()
		("help,h", "Display help message")
		("debug", "Show verbose/debug messages")
		("output,o", po::value<std::string>()->default_value("")->value_name("path"), "Output file")
		("format,f", po::value<std::string>()->default_value("text")->value_name("format"), "Output format")
		("detail,d", po::value<unsigned char>()->default_value(2)->value_name("level"), "How much detail to output");

	po::options_description positionals("Options");
	positionals.add_options()("input", po::value<std::string>(), "Input file");
	po::positional_options_description _positionals;
	_positionals.add("input", 1);
	try
	{
		po::store(po::command_line_parser(argc, argv).options(po::options_description().add(cli).add(positionals)).positional(_positionals).run(), options);
	}
	catch( const std::exception &ex)
	{
		std::cerr << ex.what() << std::endl;
		return 1;
	}
	po::notify(options);

	if (options.count("help"))
	{
		std::cout << "Usage: <input> [options]" << std::endl
				  << cli << std::endl;
		return 0;
	}

	if (!options.count("input"))
	{
		std::cerr << "No input file specified." << std::endl;
		return 1;
	}

	const std::string path = options.at("input").as<std::string>();
	const std::string output = options.at("output").as<std::string>();
	const std::string format = options.at("format").as<std::string>();
	const unsigned char detailLevel = options.at("detail").as<unsigned char>();
	if (format != "text" && (output.empty() || output == "/proc/stdout"))
	{
		std::cerr << "No output file specified." << std::endl;
		return 1;
	}
	try
	{
		std::cout << "Dump of " << path << ":\n";

		File file;
		file.readPath(path);

		std::cout << "Header: \nmagic: " << file.magic() << "\n";
		const unsigned char _type = file.header.type;
		const std::string type = (_type < maxFileType) ? fileTypes.at(_type) : "Unknown (" + std::to_string(_type) + ")";
		std::cout << "type: " << type << "\nversion: " << std::to_string(file.version()) << std::endl;

		if (file.type() == FileType::NONE)
		{
			std::cout << "No data" << std::endl;
			return 0;
		}

		std::ostream &out = output.empty() ? std::cout : *(new std::ofstream(output));

		if (format == "gv" || format == "dot")
		{

			switch (file.type())
			{
			case FileType::NONE:
				out << "";
				break;
			case FileType::NETWORK:
				out << "digraph net_" << file.network->id << " {\n";
				for (auto &[id, neuron] : *file.network)
				{
					out << "\tn_" << id << " -> {";
					for(size_t i = 0; i < neuron.outputs.size(); i++)
					{
						if (i != 0)
						{
							out << ',';
						}
						out << "n_" << neuron.outputs[i].neuron;
					}
					out << "}\n";
				}
				out << "}";
				break;
			case FileType::PARTIAL:
				out << "Not supported" << std::endl;
				break;
			case FileType::FULL:
				out << "Not supported" << std::endl;
				break;
			}
		}

		else if (format == "text")
		{

			if (file.type() == FileType::NONE)
			{
				out << "";
			}
			if (file.type() == FileType::NETWORK)
			{
				out << "Network " << file.network->id << " (" << file.network->size() << " neurons)" << (detailLevel > 0) << std::endl;
				out.flush();
				for (auto &[id, neuron] : *file.network)
				{
					uint8_t type = static_cast<unsigned char>(neuron.type);
					out << "\tNeuron " << id << " (";
					out << (type < maxNeuronType) ? neuronTypes.at(type) : "Unknown Type (" + std::to_string(type) + ")";
					out << ", " << neuron.outputs.size() << " outputs)" << (detailLevel > 1 && neuron.outputs.size() > 0 ? ":" : "");
					if (detailLevel > 1)
					{
						for(size_t i = 0; i < neuron.outputs.size(); i++)
						{
							const Neuron::Connection &conn = neuron.outputs[i];
							if (i != 0)
							{
								out << ',';
							}
							out << "n_" << conn.neuron;
							if (detailLevel > 2)
							{
								out << " (" << conn.strength << "," << conn.plasticityRate << "," << conn.plasticityThreshold << "," << conn.reliability << ")";
							}
						}
					}

					out << '\n';
				}
			}
			if (file.type() == FileType::PARTIAL)
			{
				out << "Not supported" << std::endl;
			}
			if (file.type() == FileType::FULL)
			{
				out << "Not supported" << std::endl;
			}
		}

		else
		{
			std::cerr << "Format not supported" << std::endl;
		}

		out.flush();
		return 0;
	}
	catch (std::exception &err)
	{
		std::cerr << err.what() << std::endl;
		return 1;
	}
}
