int main( int argc, char **argv )
{
  int sox_argc;
  char **sox_argv;
  int exit_code;
  
  lsx_init_console();
  lsx_init_commandline_arguments(&sox_argc, &sox_argv);

  exit_code = sox_main(sox_argc, sox_argv);
  
  lsx_uninit_console();
  lsx_free_commandline_arguments(&sox_argc, &sox_argv);

  return exit_code;
}
