#ifndef	_COMPOSITEDOCUMENT_HPP
#define	_COMPOSITEDOCUMENT_HPP

#include	<vector>
#include  <string>

using namespace std;
/**
 * @author richard lum
 * @date   20120217
 *
 * Purpose: map an overall line number to a source document and local line number given that
 * include files can be nested.  Given this nesting and the fact that the same file can be
 * included multiple times from multiple different parent documents, a snapshot of the
 * document hierarchy is given (file, line no at each level).
 *
 * The overall line number (composite line number) is the line number associated with
 * the buffer that contains all included files.
 *
 * Terminology: A BookMark is a document name and local line number
 *    A DocStack is a collection of BookMarks reflecting the include hierarchy at given point in time
 *    Composite Document is a collection of DocStack's representing the change points in the overall buffered document.
 *      Essentially, whenever there is a change in the DocStack size, we want a snapshot in the composite document
 *    Composite Line number is the line number associated with the buffer that contains
 *      root document and all included files expanded into a single buffer.(vs local line number)
 *    Index is the byte offset into the expanded files in  buffer.  We keep
 *      track of the byte offset of all newlines to allow mapping of index to
 *      composite line numbers. This in turn enables mapping to a Composite Document
 *      (eg stack of book marks for any given byte off
 *
 *  Usage:
 *     initialize by creating a CompositeDocument
 *     call includeFile for each document (including the initial root document)
 *     call endIncludeFile when each document is finished
 *     call incr_line, every time a newline is encountered as we load it into the composite document
 *     use strStackFromCompositeLineNumber to get a string representation of the document stack that is
 *      equivalent to the line number in the composite document
 *     Note:
 *      xp_position[xp_stack] is used within xp_parser as a ptr into xp_file
 *      xp_get_whereami_key leverages this but some methods make local copies for processing
 *      convert_whereami_key_to_string(xp_get_whereami_key())  placed where
 *        you want source file info.  Valid during parsing stage, before
 *        sipp creates second scenario in xp_file for reponses.  Currently no direct
 *        tie between xp_file location information and generated sipp datastructures
 *        during playback.
 *
 *   Note: when looking at composite line number and local line numbers, it may
 *   appear at first glance that the local line number do not add up to the
 *   expected composite line number. However recall that each include line
 *   appears the including file but not the composite document.  That include
 *   line is replaced by all the lines in the included file so that line number
 *   cannot simply be added up for a given set of bookmarks to get a composite
 *   line number.  Also, as include documents are done, they are taken of the
 *   stack but their contents have contributed to the composition line number.
 *
 *   @Invariant = composite line number of the previous stack + change in the
 *   top document line number - 1 =   new composite line number.
 *
 *    All line numbers are 1 based so that they line up to what you would
 *    expect to see in a typical editor.
 *    1.Include files only impact DocStack images collected by CompositeDocument.
 *    2.newlines only increment the top document and composite line number.
 *      newlines come from only a single file source.
 *
 */

class CompositeDocument {

  // file and line number record
  class BookMark {
  private:
    string doc;
    int localLine;
  public:
    BookMark(string name) :
      doc(name), localLine(1) {
    }
    int localLineNumber() {
      return localLine;
    }
    int incr_localline() {
      localLine++;
      return localLine;
    }
    string docname() {
      return doc;
    }
    void reduceLocalLineBy(int delta) {
      localLine -= (delta);
    }
  }; // inner class BookMark

  // collection of bookmarks reflecting include heirarchy of documents
  class DocStack {
  private:
    vector<BookMark> docs;
    int compositeLineNumber;
  public:
    DocStack(string docname);
    DocStack();
    ~DocStack();
    //DocStack(const DocStack*& source);
    void push(string newdoc);
    string pop();
    int nextline(); // returns local line number
    string showStack();
    string topDoc();
    int getCompositeLineNumber();
    int getDocsInStack();
    void reduceBothLineNumBy(int delta);
    // void incrCompositeLineNumber();
  }; // inner class DocStack

private:
  // collection of document stack images
  vector<DocStack> compositeDocument;
  // byte offset of newlines in composite document
  vector<int> lineToOffset;
  DocStack docStackFromCompositeLineNumber(int compositeLineNumber);

public:
  CompositeDocument();
  ~CompositeDocument();
  int incr_line(int index);
  void includeFile(string includeFileName);
  void endIncludeFile();
  int getCompositeLineNumber();
  string strStackFromCompositeLineNumber(int compositeLineNumber);
  string getCurrDoc();
  int currStackSize();
  int getQtyStacks();
  string dumpStacks();
  int compositeLineNumberFromIndex(int index);
  string strStackFromIndex(unsigned int index);
  void showLineOffsetMap();
  bool checkNewLineSynch(const char* xp_file);
  void reset();  // make like this object is newly made

// routines for testing only
  DocStack getCurrStack();
  vector<int> getLineOffsetMap();
};

#endif	
