/**
 * @file main2.cpp
 * @brief Computation of adaptive weights in a square window
 * @author Laura F. Julià <fernandl@imagine.enpc.fr>
 *         Pascal Monasse <monasse@imagine.enpc.fr>
 */


#include "show_weights.cpp"
#include "cmdLine.h"
#include "io_png.h"
#include <iostream>

/// Usage Description
static void usage(const char* name, int radius, float gamma_s, float gamma_p) {
	std::cerr <<"Show weights\n"
		<< "Usage: " << name << " [options] im1.png x y out.png [im2.png disp] \n\n"
		<< "Options (default values in parentheses)\n\n"
		<< "Adaptive weights parameters:\n"
		<< "    -R radius: radius of the window patch ("
		<<radius << ")\n"
		<< "    -s gamma_s: gamma for color similarity ("
		<<gamma_s << ")\n"
		<< "    -p gamma_p: gamma for distance ("
		<<gamma_p << ")\n"
		<< "    -c: combines two weights with product if it is 1 (0)\n"
		<< std::endl;
}

/// Main Program
int main(int argc, char *argv[])
{
	CmdLine cmd;

    int radius = 17, combine=0;
    float gamma_s=12, gamma_p=17.5;

	cmd.add( make_option('R', radius) );
	cmd.add( make_option('s', gamma_s) );
	cmd.add( make_option('p', gamma_p) );
	cmd.add( make_option('c', combine) );

	try {
		cmd.process(argc, argv);
	} catch(std::string str) {
		std::cerr << "Error: " << str << std::endl<<std::endl;
		usage(argv[0], radius, gamma_s, gamma_p);
		return 1;
	}
	if(!(argc==5 && combine==0) && !(argc==7 && combine==1)) {
		usage(argv[0], radius, gamma_s, gamma_p);
		return 1;
	}

	int X, Y;
	if(! ((std::istringstream(argv[2])>>X).eof() &&
		  (std::istringstream(argv[3])>>Y).eof())) {
        std::cerr << "Error reading x or y" << std::endl;
        return 1;
	}

	int disp;
	if(combine==1 && !((std::istringstream(argv[6])>>disp).eof()) ) {
        std::cerr << "Error reading disparity" << std::endl;
        return 1;
	}

    if(combine==0)
        show_weights(argv[1], X, Y, radius, gamma_s, gamma_p, argv[4]);
    if(combine==1)
        show_weights2(argv[1], argv[5], X, Y, radius, gamma_s, gamma_p, argv[4], disp);

	return 0;
}
