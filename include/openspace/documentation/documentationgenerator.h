/*****************************************************************************************
 *                                                                                       *
 * OpenSpace                                                                             *
 *                                                                                       *
 * Copyright (c) 2014-2023                                                               *
 *                                                                                       *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this  *
 * software and associated documentation files (the "Software"), to deal in the Software *
 * without restriction, including without limitation the rights to use, copy, modify,    *
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to    *
 * permit persons to whom the Software is furnished to do so, subject to the following   *
 * conditions:                                                                           *
 *                                                                                       *
 * The above copyright notice and this permission notice shall be included in all copies *
 * or substantial portions of the Software.                                              *
 *                                                                                       *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,   *
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A         *
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT    *
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF  *
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE  *
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                                         *
 ****************************************************************************************/

#ifndef __OPENSPACE_CORE___DOCUMENTATIONGENERATOR___H__
#define __OPENSPACE_CORE___DOCUMENTATIONGENERATOR___H__

#include <openspace/json.h>
#include <string>
#include <vector>

namespace openspace {

/*
 * This abstract class is used for instances when another class has the ability to
 * generate a Handlebar generated documentation file that contains valuable information
 * for the user. Instances of this are the DocumentationEngine results, the list of
 * Property%s generated by the Scene, or the FactoryEngine results. The documentation is
 * generated through the writeDocumentation method.
 *
 * The concrete subclass needs to overload the generateJson class that will return the
 * Json that is parsed by Handlebar to generate the webpage.
 *
 * A list of Handlebar templates, the variable name for the result of the generateJson
 * method, and the Javascript file that contains additional logic is passed in the
 * constructor.
 */
class DocumentationGenerator {
public:
    static const std::string NameTag; 
    static const std::string DataTag; 
    /**
     * The constructor that is used to set the member variables later used in the
     * writeDocumentation method.
     *
     * \param name The name of the written documentation
     * \param jsonName The variable name of the value generated by the generateJson
     *
     * \pre name must not be empty
     * \pre jsonName must not be empty
     * \pre javascriptFilename must not be empty
     */
    DocumentationGenerator(std::string name, std::string jsonName);

    /// Default constructor
    virtual ~DocumentationGenerator() = default;

    //getter for identifier
    std::string jsonName();

    /**
     * This abstract method is used by concrete subclasses to provide the actual data that
     * is used in the documentation written by this DocumentationGenerator class. The JSON
     * string returned by this function will be stored in the variable with the
     * `jsonName` as passed in the constructor.
     *
     * \return A JSON script that is parsed and stored in the variable passed in the
     * DocumentationGenerator constructor
     */
    virtual std::string generateJson() const = 0;

private:
    const std::string _name;
    const std::string _jsonName;
};

} // namespace openspace

#endif // __OPENSPACE_CORE___DOCUMENTATIONGENERATOR___H__
