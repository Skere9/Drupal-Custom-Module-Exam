# Drupal-Custom-Module-Exam
Custom module - certification exam with randomized questions, Schema and Forms API connections

***************************************
* Exam Module by DatabaseTraining.com *
***************************************

This is the Exam Module by DatabaseTraining.com

Author: Steve O'Hearn
Date Created: Summer 2013

*************************
* INSTALLING THE MODULE *
*************************

Install the Exam module like any other Drupal module.  However, 
you're not done until you also install the exam data.

****************************
* INSTALLING THE EXAM DATA *
****************************

To install the exam data, use something like PHPMyAdmin to run
the SQL scripts in the file 2013_08_16_Create_Initial_Exam_Database.txt.

Then, use the new "rebuild menu" menu option to rebuild the menu and 
add the new exam(s) as menu options.

*********************
* EDITING THE THEME *
*********************
To install it, you must also edit the accompanying theme.  Add 
the following code to the beginning of the *.css file for the theme.
This is required to ensure that the source code contained in the 
exams will display correctly.


/* Added for DatabaseTraining.com Exam */

.exam_code {
  color:black;
  font-style:normal;
  font-family: monospace;
}

.exam_code_inline {
  color:black;
  font-style:normal;
  font-family: monospace;
}

.exam_unused{
    display:inline; /*  */
}

th.exam_numcol {
  text-align: right;    
}

td.exam_numcol {
  text-align: right;    
}

td.exam_numcolctr {
  text-align: center;    
}

**************************
* END OF README.TXT FILE *
**************************
