/**
 * A re-implementation of the synti2 GUI. I hope this will be better
 * than the first piece of crap.
 *
 */



int main(int argc, char **argv) {
  int retval;

#if 0
  pt = new synti2::Patchtool(patchdes_fname);
  pbank = pt->makePatchBank(16);
  midimap = new synti2::MidiMap();
 
  init_jack_or_die();

  /* Make an edit window for our custom kind of patches. */
  (pt->exposePatchDescr())->headerFileForC(std::cout);
  Fl_Window *window = build_main_window(pt->exposePatchDescr());

  widgets_to_reflect_reality();

  window->show(argc, argv);

  retval = Fl::run();

  jack_ringbuffer_free(global_rb);
  jack_client_close(client);
  
  if (pbank != NULL) free(pbank);
  if (pt != NULL) free(pt);
#endif
  return 0;
}
