ambnb
=====

DESCRIPTION:

__ambnb__ is a freeBSD kernel module which allow users to manage the backlight of their screen via the sysctl interface. It was originally written by Patrick Lamaiziere <patfbsd@davenulle.org> in 2009. 
It works at least for macbookpro from 1,2 up to 4,2 (not tested for other models. 
I modifyied it slight a bit in order to make it usable by unpriviledged users and to move it from hw branch to dev branch. 

INSTALLATION:

Retrieve the files:

	git clone https://github.com/dervishe-/ambnb.git

then go to the directory and type:

	make; make install

That's it

USE:

sysctl dev.ambnb.level=value

where value is an integer from 0 to 15

