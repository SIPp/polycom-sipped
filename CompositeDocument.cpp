#include "CompositeDocument.hpp"
#include <iostream>

#include <cstring>
#include <sstream>
#include <iomanip>
#include <cstdio>

using namespace std;

CompositeDocument::CompositeDocument()
{
}

CompositeDocument::~CompositeDocument()
{
  //cout << "deleting CompositeDocument\n";
}
// get the current compositeLineNumber
int CompositeDocument::getCompositeLineNumber()
{
  DocStack currDocStack = compositeDocument.back();
  return currDocStack.getCompositeLineNumber();
}
// input: name of the new include file
//
// create copy of current DocumentStack, push new file on top
// add new copy of Documentstack to CompositeDocument
void CompositeDocument::includeFile(string includeFileName)
{
  DocStack newDocStack;
  if (compositeDocument.size() == 0) {
    newDocStack.push(includeFileName);
  } else {
    DocStack currDocStack = compositeDocument.back();
    newDocStack = DocStack(currDocStack);
    newDocStack.push(includeFileName);
  }
  compositeDocument.push_back(newDocStack);
}

// returns local line number, does not record index (global byte offset)
// deprecated
//int CompositeDocument::incr_line() {
//  return compositeDocument.back().nextline();
//}

/**
 * Builds a map of all the newline locations
 * @param:  index is the byte offset into the global document buffer that the new line appears at
 * @return: local line number
 */
int CompositeDocument::incr_line(int index)
{
  lineToOffset.push_back(index);
  return compositeDocument.back().nextline();
}

void CompositeDocument::endIncludeFile()
{
  DocStack currDocStack = compositeDocument.back();
  DocStack newDocStack(currDocStack);
  newDocStack.pop();
  compositeDocument.push_back(newDocStack);
}

/**
 * @input: composite line number
 * @return: DocStack image of bookmarks equivalent to given line number
 *
 * start at first DocStack, get it's composite line numbe
 * if that number is > the target, stop and adjust the
 * composite line number to match target. For the
 * top bookmark in the stack, adjust the  local line number
 * by the same amount
 */
CompositeDocument::DocStack CompositeDocument::docStackFromCompositeLineNumber(
  int targetCompLineNu)
{
  DocStack target;
  unsigned int i = 0; // compositeDocument.size() -1;
  bool found = false;
  while ((i < compositeDocument.size()) && (!found)) {
    int compLineNu = compositeDocument[i].getCompositeLineNumber();
    // search for first stack with a comp lineno > target
    if ((compLineNu > targetCompLineNu)) {
      found = true;
      // always want to show the deeper stack.  As we include docs, deeper stack is later
      // as we pop include files of stack, deeper stack is the older/earlier stack
      if ((i>0) && (compositeDocument[i].getDocsInStack() < compositeDocument[i-1].getDocsInStack())&&
          (compositeDocument[i-1].getCompositeLineNumber() == targetCompLineNu )) {
        target = DocStack(compositeDocument[i-1]);
      } else {
        // have stack with compositeLineNumber>target, decr comp and local until target reached
        target = DocStack(compositeDocument[i]);
        // adjust comp and local line numbers so that comp line = target
        int delta = compLineNu - targetCompLineNu;
        if (delta < 0)
          cout << "targetlinenu > compLineNu from stack" << endl;
        target.reduceBothLineNumBy(delta);
      }
    } else {
      i++;
    }
  }
  if (!found)
    cout << "exceeded compositeDocument size\n";

  return target;
}

string CompositeDocument::strStackFromCompositeLineNumber(int targetCompLineNu)
{
  DocStack target = docStackFromCompositeLineNumber(targetCompLineNu);
  return target.showStack();
}

// get the name of the document that is currently being processed = document on top of stack
string CompositeDocument::getCurrDoc()
{
  return compositeDocument.back().topDoc();
}

// get the most recent image of the stack of documents that is currently being processed
CompositeDocument::DocStack CompositeDocument::getCurrStack()
{
  return compositeDocument.back();
}

// how many bookmarks in current/latest DocStack image
int CompositeDocument::currStackSize()
{
  return getCurrStack().getDocsInStack();
}

// how many DocStack Images have we collected
int CompositeDocument::getQtyStacks()
{
  return compositeDocument.size();
}

// show all stacks that we have collected
string CompositeDocument::dumpStacks()
{
  string result ="";
  int i;
  char buf[256];
  for (i = 0; i < getQtyStacks(); i++) {
    sprintf(buf,"%d",compositeDocument[i].getCompositeLineNumber());
    result += buf;
    result +=  "  ------------------\n";
    result += compositeDocument[i].showStack();
    result += "\n";
  }
  return result;
}

// show byte offset of all newlines in the composite document (xp_file)
void CompositeDocument::showLineOffsetMap()
{
  cout << "--------showing line to index map (Byte offset of newlines)----"
       << endl;
  for (unsigned int i = 0; i < lineToOffset.size(); i++) {
    cout << "\t" << setw(4) << i << ", " << setw(6) << lineToOffset[i];
    if (!((i + 1) % 5))
      cout << "\n";
  }
  cout << endl;

}

//
vector<int> CompositeDocument::getLineOffsetMap()
{
  return lineToOffset;
}
/**
 * @param: given a byte offset into the composite document
 * @return: the composite line number of within the composite document
 */
int CompositeDocument::compositeLineNumberFromIndex(int index)
{
  unsigned int i = 0; // incr line no until byte
  // lineToOffset is 0 based, so actual lineno is +1
  while ((lineToOffset[i] < index) && (i < lineToOffset.size())) {
    i++;
  }
  if (i >= lineToOffset.size())
    cout << "Search for line from index failed";
  return i+1;  //zero based linetooffset since we just push onto vector
  // line number is +1

}

/**
 * @input index = byte offset into composite document(xp_file)
 * @output stringified equivalent DocStack at given byte offset
 * @precondition line number has less than maxdigits digits
 */
string CompositeDocument::strStackFromIndex( int index)
{
  const int maxdigits = 256;
  char buf[maxdigits];
  string result = "";
  int compLineNumber = compositeLineNumberFromIndex(index);
  if (index > lineToOffset[lineToOffset.size()-1]) {
    // we are probably reporting an erro while building xp_file
    // give the latest stack since we havent finished building
    // current stack yet
    //result += strStackFromCompositeLineNumber(lineToOffset[lineToOffset.size()-1]);
    result += getCurrStack().showStack();
  } else {
    result += strStackFromCompositeLineNumber(compLineNumber);
  }
//cout << __FILE__ << __LINE__ << result << endl;
  sprintf(buf, "%d", index);
  result += "Byte offset = ";
  result += buf;
  result += ",\t";
  sprintf(buf, "%d", compLineNumber);
  result += "Line offset ";
  result += buf;
  result += "\n";
  return result;
}

// walk collection of newline byte offsets in buffer file and confirm all
// saved newlines are found.
bool CompositeDocument::checkNewLineSynch(const char* xp_file)
{
  unsigned int i = 0;
  while (i < lineToOffset.size()) {
    if (xp_file[lineToOffset[i]] == '\n') {
      i++;
    } else {
      cout << i << " / " << lineToOffset.size() << " newline ";
      cout << "mismatch at offset ";
      cout << lineToOffset[i];
      cout << ", found: " << xp_file[lineToOffset[i]] << endl;
      const int bufsize=50;
      char buf[bufsize];
      strncpy(buf,xp_file+lineToOffset[i]-10, bufsize-1);
      cout << "xp_file around mismatch: " << endl;
      cout << buf << endl;
      cout << "         *"<< endl;
      return false;
    }
  }
  return true; // only get here if all the saved newline positions are found in xp_file at the correct offset
}

void CompositeDocument::reset()
{
  while (compositeDocument.size() > 0) {
    compositeDocument.pop_back();
  }
  while (lineToOffset.size() > 0) {
    lineToOffset.pop_back();
  }
}

/////////////////////////////////////
// nested class DocStack


CompositeDocument::DocStack::DocStack(string docname) :
  compositeLineNumber(1)
{
  string rootdoc(docname);
  push(rootdoc);
}

// note that line number is 1 based
CompositeDocument::DocStack::DocStack() :
  compositeLineNumber(1)
{

}

CompositeDocument::DocStack::~DocStack()
{
  while (docs.size() != 0) {
    docs.pop_back();
  }
}

//  each line increments the compoiste linenumber counter in the composite document
//  and only the local line number in the top document on the document stack.
//  Containing Documents remain on their the current local line number ( the xi include line)
int CompositeDocument::DocStack::nextline()
{
  int newlinenumber = docs.back().incr_localline();
  compositeLineNumber++;
  return newlinenumber;
}

int CompositeDocument::DocStack::getCompositeLineNumber()
{
  return compositeLineNumber;
}

// add a bookmark to the documentstack
void CompositeDocument::DocStack::push(string doc)
{
  BookMark mark(doc);
  docs.push_back(mark);
}

// remove a bookmark from the document stack
// return the name of the file popped
string CompositeDocument::DocStack::pop()
{
  BookMark amark = docs.back();
  string docname = amark.docname();
  docs.pop_back();
  return docname;
}

// stringify the document stack, (list of bookmarks)
string CompositeDocument::DocStack::showStack()
{
  stringstream ss;
  int i;
  string docstack = "";
  for (i = docs.size() - 1; i >= 0; i--) {
    BookMark abookmark = docs[i];
    ss.clear();
    ss.str("");
    ss << ":";
    ss << (abookmark.localLineNumber());
    ss << endl;
    docstack += abookmark.docname();
    docstack += ss.str();
    //docstack = abookmark->docname + string("\t") +  ss.str() + string("\n");
    //cout << docstack;
  }
  return docstack;
}

// what is the current file that is on top of the document stack
string CompositeDocument::DocStack::topDoc()
{
  if (docs.size() > 0)
    return docs.back().docname();
  else
    return "";
}

// how many files are in the document stack
int CompositeDocument::DocStack::getDocsInStack()
{
  return docs.size();
}

/**
 * @input: number of lines to reduce Docstack to reach desired line number
 * @precondition: delta is zero/positive.  This document stack must have a composite
 *  line number larger than/equalto the target line number, otherwise we are using the
 *  wrong DocStack as the basis for getting correct image at target line number
 */
void CompositeDocument::DocStack::reduceBothLineNumBy(int delta)
{
  if (delta<0) {
    cerr << "DocStack received negative adjustment request, value = ";
    cerr << delta;
    cerr << " Reported Document locations will be incorrect!!! ";
  }
  compositeLineNumber -= delta;
  docs.back().reduceLocalLineBy(delta);
}





