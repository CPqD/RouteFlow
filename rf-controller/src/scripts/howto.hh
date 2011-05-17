/** \defgroup utility NOX Utilities
 *
 * These utilities automates some of the basic howtos in NOX.
 * They also make some of the common operations easier.
 */

/** \page Howto Basic Howto's in NOX
 * <UL>
 * <LI> \ref new-c-component </LI>
 * <LI> \ref new-event </LI>
 * <LI> \ref parse-component-arguments </LI>
 * </UL>
 *
 * <HR>
 *
 * \section new-c-component Creating new C/C++ component 
 * @author ykk
 * @date December 2009
 *
 * To create a new C/C++ component, the following has to be done:
 * <OL>
 * <LI>Create a new directory in coreapps, netapps or webapps</LI>
 * <LI>Add the appropriate C and header files with meta.json and Makefile.am</LI>
 * <LI>Add the new directory to configure.ac.in</LI>
 * <LI>Rerun ./boot.sh and ./configure</LI>
 * </OL>
 * A sample of the files needed in step 2 is given in coreapps/simple_c_app.
 * nox-new-c-app.py is a script that uses coreapps/simpe_c_app to create a 
 * new C/C++ component (executing step 1 to 3).  The script takes in the new
 * component's name, and has to be run in coreapps, netapps or webapps.
 *
 * <HR>
 *
 * \section new-event Creating new event 
 * @author ykk
 * @date December 2009
 *
 * To create a new event, the following is required:
 * <OL>
 * <LI>Create definition of event, inheriting from the Event class.
 *     The get_static_name function must be redefine (else parent event
 *     will be said to be missing).</LI>
 * <LI>Call register_event in configure function of a component
 *     register the event.</LI>
 * </OL>
 * Something to note is that it is preferably add the event 
 * to src/etc/nox.xml.  However, the event must have at least one
 * subscription.
 * 
 * <HR>
 *
 * \section parse-component-arguments Parsing arguments for components. 
 * @author ykk
 * @date December 2009
 *
 * NOX allows components to get commandline arguments, through Configuration.
 * In the configure function, the configuration is passed in as an argument.
 * To get the argument string for the components, use<BR>
 * <PRE>string arg = c->get_argument()</PRE><BR>
 * 
 * If you use the convention of separating each argument with a delimiter, and
 * the argument identifier and value with another delimiter, you can use<BR>
 * <PRE>const hash_map<string,string> argmap = c->get_arguments_list(',','=');</PRE><BR>
 * <PRE>hash_map<string,string>::const_iterator i = argmap.find("interval");</PRE><BR>
 * <PRE>if (i != argmap.end())</PRE><BR>
 * <PRE>   interval = atoi(i->second.c_str());</PRE><BR>
 * Here, we have used comma and equal as the delimiters respectively. 
 * This example from allows the component to be called with arguments like this:<BR>
 * <PRE>./nox_core switchrtt=interval=600</PRE><BR>
 */
