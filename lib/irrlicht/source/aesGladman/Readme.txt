A File Encryption Utility - VC++ 7.1 project Instructions

1.	Unzip the enclosed files into a suitable VC++ project directory.
2.	Obtain the bzip2 source code from http://sources.redhat.com/bzip2/
	and unzip the files into the bzip2 sub-directory.
3.	Compile the bzip2 project to give a static library
4.	Compile the encfile project.
5.	The executable encfile.exe is now ready for use:

		enfile password filename
	
	If the filename does not have the extension 'enc', it is assumed to 
	be a normal file that will then be encrypted to a file with the same
	name but with an added extension 'enc'.
	
	If the filename has the extension 'enc' its is assumed to be an 
	encrypted file that will be decrypted to a file with the same name
	but without the 'enc' extension.

The default HASH function is SHA1, which is set up by defining USE_SHA1 in
compiling the project.  If USE_SHA256 is defined instead then SHA256 is used.

Brian Gladman
 
	
