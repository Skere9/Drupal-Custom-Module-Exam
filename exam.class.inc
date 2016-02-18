<?php

/*
 * This class is an instance of an exam.
 * In other words, this is an exam someone has
 * decided to take.  
 * 
 */

function populateRangeCounter() {
  
  // Use this to populate a static variable for the ExamInstance class
  // Creates a range of letters in an array
  // NOTE:  Currently unused

  $rangeCounter = 1;
  foreach(range('A', 'Z') as $letter) {
    $vRangeArray[$rangeCounter] = $letter;
        $rangeCounter++;
  }
  
  return $vRangeArray;
}

function getLetterForNumber($pNumber) {
  return ExamInstance::$sRangeLetters[$pNumber];
}

class ExamInstance {

  // Static variables
  static $total_Exams_In_Play = 0; // int
  // Create a range of letters to be assigned to a single question's set of answers
  // X is a placeholder for the Zero'th value.  No particular reason I 
  // did this, just a personal preference to accomodate the looping 
  // that occurs later
  static $sRangeLetters = array('X', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I'); 

  // Member variables
  // $ei_exam: array of info for one exam
  var $ei_exam;
  // $ei_topic: array of topics for one exam.  Initializing is required,
  // the first row of useful data skips the 0 row and starts with 1.
  var $ei_topic     = array('0');
  // $ei_qa: array of questions and answers for one exam.
  var $ei_qa        = array('0');   
  var $ei_answerSet = array('0');
  
  // The exam is created with the assumption it is not yet graded.  
  // This will be used to populate the 'graded' column of the 
  // exam_instance and its internally stored array equivalent.
  var $vGraded = 0; 

// Static public function
  
static public function storeMessage($pMessage, $pSource) {
  
  // I use this to trouble-shoot the system from time to time.
  // This app has calls to this function all over the place,
  // storing data to support debugging efforts during development.
  // When not doing that, I comment out the insert statements
  // to improve performance.
  
   global $user;
  
  $eiid = db_insert('exam_system_messages')
      ->fields(array(
          'esm_id'      => null
        , 'esm_message' => $pMessage
        , 'esm_source'  => $pSource . ' by user ' . $user->uid
        , 'created' => REQUEST_TIME
      ))
      ->execute();
  
  return null;
}  
  
static public function getListingOfExams() {

    ExamInstance::storeMessage('SKERE - Here.', 'getListingOfExams');
    
    global $user;
    $vNumberOfColumns = 3;
    $vContent = '';
    
    // This is a complex query, but for a very good reason.
    // The primary purpose of this query is to list the exams.
    // However, I wanted to include two additional bits of data.
    // One, the total number of questions on the exam.  That requires
    // a join to the questions table.
    // Two, a blurb about any exam the user may already have taken.  
    // That is why the query is so complex, because I had to join to
    // a single query that itself is a correlated subquery, which 
    // hits the "exam_instance" table and finds all exams for this 
    // user that are unfinished, by checking the max primary key value 
    // with a "graded" value of 0.  That assumes, of course, that
    // the exam_instance_id value is perpetually incrementing, 
    // which may not be the smartest thing to do - the most logical
    // would be to go by the created date.  But I wanted to use the
    // MAX value to connect the queries with IN, so I went 
    // with exam_instance_id.  Joining by created date wouldn't be 
    // so smart, I don't think.
    //
    $result = db_query(
      'SELECT   e.exam_id
              , e.exam_title
              , e.exam_description
              , eiv.exam_instance_id
              , IFNULL(eiv.graded,\'X\') graded
              , FROM_UNIXTIME(eiv.created, \'%a %b %D, %Y %h:%i:%s %p\') created              
              , COUNT(q.question_id) tot_q
       FROM     exam AS e 
                LEFT JOIN question AS q
                  ON e.exam_id = q.exam_id
                LEFT JOIN (SELECT   a.exam_instance_id
                                  , a.user_id
                                  , a.exam_id
                                  , a.graded
                                  , a.totalQuestions
                                  , a.created
                           FROM     exam_instance a
                           WHERE  a.showInHistory = 1
                             AND  a.exam_instance_id IN 
                             (SELECT MAX(b.exam_instance_id)
                              FROM exam_instance b JOIN exam_instance a
                              ON b.exam_instance_id = a.exam_instance_id
                              AND b.graded = 0
                              GROUP BY b.exam_id)
                             AND  a.user_id = :uid) eiv
                  ON e.exam_id = eiv.exam_id
       GROUP BY e.exam_title
              , e.exam_description
              , eiv.exam_instance_id
              , eiv.graded
       ORDER BY e.exam_id',
       array(':uid' => $user->uid)        
        );

    $vContent .= '<table>';

    $vContent .=  
        '<tr>'
      . '<th>Exam</th>'
      . '<th>Description</th>'
      . '<th>Action</th>'
      . '</tr>';

    if ($result->rowCount() == 0) {
      $vContent .= 
          '<tr><td colspan=' 
        . $vNumberOfColumns 
        . ' align=center>No exams are available.</td></tr>';
    } else {

      foreach ($result as $row) {
        $vExamTitleFormatted = str_replace(' ', '-', strtolower($row->exam_title));
        $vContent .=
            '<tr>'
          . '<td>' . $row->exam_title . '</td>'
          . '<td>' . $row->exam_description . '<br>' . $row->tot_q . ' questions.</td>';
        // Check if the test is already started.  
        // If not, display an "initiate" option.
        // If yes, display a "continue" option.
        if ($row->graded == '0') {
          // Test already started.  Display "resume".
          $vContent .=
                '<td>You started this exam<br>' . $row->created . '<br>'
              . '<a href="?q=exam/continue/' 
              . $vExamTitleFormatted 
              . '/' . $row->exam_instance_id . '">CONTINUE</a></td>';   
        } else {
          // Test not yet begun.  Display "initiate".
          $vContent .=
                '<td><a href="?q=exam/list/' 
              . $vExamTitleFormatted 
              . '">START</a></td>';       
        }
        $vContent .= '</tr>';
      }
    }
  
    $vContent .= '</table>';
    
    return $vContent;
  }
  
  /* 
   * Returns a tabular summary of one completed exam instance
   * Shows the number of questions answered incorrectly
   * by topic, in descending order.
   * 
   */
  static public function getExamResultsByTopic($pExamInstanceID) {

    $vColSpan = 5; // Used in the error message below.
    $vContent = '';
    
    ExamInstance::addJQueryTablesorter();
    
    $vContent .= '<table>';
    
    // Display basic exam data
    
    $result = db_query(
        'SELECT ei.exam_title as exam_title
              , ei.grade as exam_grade
              , ei.numberCorrect as numberCorrect
              , ei.totalQuestions as totalQuestions
              , FROM_UNIXTIME(ei.created, \'%a %b %D, %Y %h:%i:%s %p\') as exam_started
         FROM   exam_instance ei
         WHERE  ei.exam_instance_id = :eiid'
       , array(':eiid' => $pExamInstanceID)
    );
    // Only one row should be returned
    foreach ($result as $row) {
      $vContent .=
            '<tr>'
          . '<td colspan=5>'
          . 'Exam: ' . $row->exam_title . '<br>'
          . 'Started: ' . $row->exam_started . '<br>'
          . 'Grade: ' . $row->exam_grade . '% out of 100.00 (' . $row->numberCorrect . ' correct of ' . $row->totalQuestions . ' questions)<br>'
          . '</td>'
          . '</tr>';
    }
    $vContent .= '</table>';
    
    $vContent .= '<table id="theTable" class="tablesorter" cellspacing="1">';
    // Query to return number of questions correct
    $result = db_query(
        
        'SELECT   ti.topic_title
                , COUNT(CASE WHEN qi.answeredCorrectly = 1 THEN \'X\' END) AS correct
                , COUNT(CASE WHEN qi.answeredCorrectly = 0 THEN \'X\' END) AS wrong
                , COUNT(CASE WHEN qi.answeredCorrectly IS NULL THEN \'X\' END) AS unanswered
           FROM   question_instance qi JOIN topic_instance ti
                  ON qi.topic_instance_id = ti.topic_instance_id
           WHERE  qi.exam_instance_id = :eiid
         GROUP BY ti.topic_title
         ORDER BY ti.topic_instance_id'
       , array(':eiid' => $pExamInstanceID)
    ); 
  
    $vContent .=
        '<thead>'
      . '<tr>'
      . '<th>#</th>'
      . '<th>Topic</th>'
      . '<th class=exam_numcolctr>Correct</th>'
      . '<th class=exam_numcolctr>Incorrect</th>'
      . '<th class=exam_numcolctr>Unanswered</th>'
      . '</tr>'
      . '</thead>';

    $vContent .= '<tbody>';
    
    $vCounter = 0;
    foreach ($result as $row) {
      $vCounter++;
      $vContent .=
          '<tr>'
        . '<td>' . $vCounter . '</td>'
        . '<td width=200>' . $row->topic_title . '</td>'
        . '<td class=exam_numcolctr>' . $row->correct    . '</td>'
        . '<td class=exam_numcolctr>' . $row->wrong      . '</td>'
        . '<td class=exam_numcolctr>' . $row->unanswered . '</td>'
        . '</tr>';
    }
    if ($vCounter ==0) {
      // No records were found, display an appropriate message
      $vContent .=
          '<tr><td colspan=' . $vColSpan . '>No such exam exists.</td></tr>';
    }

    $vContent .= '</tbody>'
               . '</table>';
        
    return $vContent;
  } // static public function getExamResultsByTopic()
  
  /* 
   * Returns a personalized report on one completed exam instance
   */
  static public function getExamResultsPersonalizedReport($pExamInstanceID) {
    
    ExamInstance::storeMessage('Just entered', 'getExamResultsPersonalizedReport');

    $vContent = '';
       
    // Display basic exam data
    
    $result = db_query(
        'SELECT ei.exam_title as exam_title
              , ei.grade as exam_grade
              , ei.numberCorrect as numberCorrect
              , ei.totalQuestions as totalQuestions
              , FROM_UNIXTIME(ei.created, \'%a %b %D, %Y %h:%i:%s %p\') as exam_started
         FROM   exam_instance ei
         WHERE  ei.exam_instance_id = :eiid'
       , array(':eiid' => $pExamInstanceID)
    );
    // Only one row should be returned
    foreach ($result as $row) {
      $vContent .=
            '<table><tr>'
          . '<td>'
          . 'Exam: ' . $row->exam_title . '<br>'
          . 'Started: ' . $row->exam_started . '<br>'
          . 'Grade: ' . $row->exam_grade . '% out of 100.00 (' . $row->numberCorrect . ' correct of ' . $row->totalQuestions . ' questions)<br>'
          . '</td>'
          . '</tr></table>';
    }
    
    // Query to return number of questions correct
    $result = db_query(
        'SELECT   ti.topic_instance_id
                , ti.topic_title
                , ti.for_more_info_see
                , COUNT(CASE WHEN qi.answeredCorrectly = 1 THEN \'X\' END) AS correct                
                , COUNT(CASE WHEN qi.answeredCorrectly = 0 THEN \'X\' END) AS wrong
                , COUNT(CASE WHEN qi.answeredCorrectly IS NULL THEN \'X\' END) AS unanswered
           FROM   question_instance qi JOIN topic_instance ti
                  ON qi.topic_instance_id = ti.topic_instance_id
           WHERE  qi.exam_instance_id = :eiid
         GROUP BY ti.topic_title
         ORDER BY wrong DESC'
       , array(':eiid' => $pExamInstanceID)
    ); 

    $vContent .= '<tbody>';
    
    $vCounter = 0;
    foreach ($result as $row) {
      $vCounter++;
      $vContent .= '<h1>' . $row->topic_title . '</h1>';
    
      // Prepare query to obtain explanations for each question answered incorrectly
      $resultExplanations = db_query(
          'SELECT qi.explanation_if_wrong
           FROM   question_instance qi
           WHERE  qi.topic_instance_id = :tiid
             AND  qi.answeredCorrectly = 0
             AND  qi.exam_instance_id = :eiid
           ORDER BY qi.question_no'
         , array(':tiid' => $row->topic_instance_id
               , ':eiid' => $pExamInstanceID)
      );     
    
      // SPECIFY WHILE LOOP HERE
      foreach ($resultExplanations as $rowExplanation) {
        $vContent .=  $rowExplanation->explanation_if_wrong;
      }    
      
    $vContent .= '<h2>For further study</h2> ' . $row->for_more_info_see;

    }    
    if ($vCounter ==0) {
      // No records were found, display an appropriate message
      $vContent .=
          '<h1>No such exam exists.</h1>';
    }
        
    return $vContent;
    
  } // static public function getExamResultsPersonalizedReport ()
  
  /* 
   * Returns a personalized report on one completed exam instance
   */
  static public function getExamResultsPersonalizedReportAsTable($pExamInstanceID) {
    
    ExamInstance::storeMessage('Just entered', 'getExamResultsPersonalizedReport');

    $vColSpan = 5; // Used in the error message below.
    $vContent = '';
    
    ExamInstance::addJQueryTablesorter();
    
    $vContent .= '<table>';
    
    // Display basic exam data
    
    $result = db_query(
        'SELECT ei.exam_title as exam_title
              , ei.grade as exam_grade
              , ei.numberCorrect as numberCorrect
              , ei.totalQuestions as totalQuestions
              , FROM_UNIXTIME(ei.created, \'%a %b %D, %Y %h:%i:%s %p\') as exam_started
         FROM   exam_instance ei
         WHERE  ei.exam_instance_id = :eiid'
       , array(':eiid' => $pExamInstanceID)
    );
    // Only one row should be returned
    foreach ($result as $row) {
      $vContent .=
            '<tr>'
          . '<td colspan=' . $vColSpan . '>'
          . 'Exam: ' . $row->exam_title . '<br>'
          . 'Started: ' . $row->exam_started . '<br>'
          . 'Grade: ' . $row->exam_grade . '% out of 100.00 (' . $row->numberCorrect . ' correct of ' . $row->totalQuestions . ' questions)<br>'
          . '</td>'
          . '</tr>';
    }
    $vContent .= '</table>';
    
    $vContent .= '<table id="theTable" class="tablesorter" cellspacing="1">';
    // Query to return number of questions correct
    $result = db_query(
        'SELECT   ti.topic_instance_id
                , ti.topic_title
                , COUNT(CASE WHEN qi.answeredCorrectly = 1 THEN \'X\' END) AS correct                
                , COUNT(CASE WHEN qi.answeredCorrectly = 0 THEN \'X\' END) AS wrong
                , COUNT(CASE WHEN qi.answeredCorrectly IS NULL THEN \'X\' END) AS unanswered
           FROM   question_instance qi JOIN topic_instance ti
                  ON qi.topic_instance_id = ti.topic_instance_id
           WHERE  qi.exam_instance_id = :eiid
         GROUP BY ti.topic_title
         ORDER BY wrong DESC'
       , array(':eiid' => $pExamInstanceID)
    ); 
  
    $vContent .=
        '<thead>'
      . '<tr>'
      . '<th>#</th>'
      . '<th>Topic</th>'
      . '<th class=exam_numcolctr>Correct</th>'
      . '<th class=exam_numcolctr>Incorrect</th>'
      . '<th class=exam_numcolctr>Unanswered</th>'
      . '</tr>'
      . '</thead>';

    $vContent .= '<tbody>';
    
    $vCounter = 0;
    foreach ($result as $row) {
      $vCounter++;
      $vContent .=
          '<tr>'
        . '<td>' . $vCounter . '</td>'
        . '<td width=200>' . $row->topic_title . '</td>'
        . '<td class=exam_numcolctr>' . $row->correct    . '</td>'
        . '<td class=exam_numcolctr>' . $row->wrong      . '</td>'
        . '<td class=exam_numcolctr>' . $row->unanswered . '</td>'
        . '</tr>';
    
      // Prepare query to obtain explanations for each question answered incorrectly
      $resultExplanations = db_query(
          'SELECT qi.explanation_if_wrong
           FROM   question_instance qi
           WHERE  qi.topic_instance_id = :tiid
             AND  qi.answeredCorrectly = 0
             AND  qi.exam_instance_id = :eiid
           ORDER BY qi.question_no'
         , array(':tiid' => $row->topic_instance_id
               , ':eiid' => $pExamInstanceID)
      );     
    
      // SPECIFY WHILE LOOP HERE
      foreach ($resultExplanations as $rowExplanation) {
        $vContent .=
            '<tr>'
          . '<td colspan=' . $vColSpan . '>' . $rowExplanation->explanation_if_wrong . '</td>'
          . '</tr>';
      }    

    }    
    if ($vCounter ==0) {
      // No records were found, display an appropriate message
      $vContent .=
          '<tr><td colspan=' . $vColSpan . '>No such exam exists.</td></tr>';
    }

    $vContent .= '</tbody>'
               . '</table>';
        
    return $vContent;
    
  } // static public function getExamResultsPersonalizedReportAsTable ()
  
  /* 
   * Add code to a web pages to enable table sorting
   * Any HTML table must have the following to enable tablesorter
   *  - use of <thead> and <tbody>
   *  - <table id="theTable" class="tablesorter">
   *  - a call to this function
   * 
   * @param pExcludeWidgets
   *   1 if you want to exclude widgets.  0 if you wish to exclude them
   * 
   * @return
   *   null
   * 
   */
  static public function addJQueryTablesorter() {
       
    // Add script tags to support the 
    // "tablesorter.js" Javascript feature.
    // Assumes the presence in the HTML body of a table
    // that uses <thead> and <tbody> tags,
    // with the attributes 
    //    id="theTable" 
    //    class="tablesorter" 
    // ... in the <table> tag.
    drupal_add_js('sites/all/modules/exam/js/jquery-latest.js', 'file');
    drupal_add_js('sites/all/modules/exam/js/jquery.tablesorter.js', 'file');
    drupal_add_js('sites/all/modules/exam/js/chili-1.8b.js', 'file');
    drupal_add_js('sites/all/modules/exam/js/docs.js', 'file');
    drupal_add_js(
      '$(function($) {
         $(document).ready(function() { 
           $("#theTable").tablesorter({widgets: ["zebra"]});     
         }); 
       });'
        , 'inline');
    return null;
  }
 
  
  /* 
   * Show one particular user's 
   * exam history
   */
  static public function getExamHistoryForUser($pUserID) {
    
    // NEEDS WORK HERE
    
  }
  
  static public function getExamTitleForURL($pExamTitle) {
    // Converts any incoming string into the URL appropriate version
    return str_replace(' ', '-', strtolower($pExamTitle)); 
  
  }

  /* 
   * Get info about an exam given an exam_instance_id
   * 
   * @param $pExamInstanceID
   *   The primary key of the exam_instance_id for which we want to retrieve a grdae
   * @result
   *   An array of information showing each column value in the 
   *   exam_instance table for a given exam_instance_id.
   *   
   */
  static public function getInfoForExamInstanceID($pExamInstanceID) {
    
    global $user;

    // For a given exam_instance_id, returns a grade to an authorized user
    $result = db_query(
            'SELECT ei.exam_instance_id
                  , ei.user_id
                  , ei.exam_id
                  , ei.type
                  , ei.exam_title
                  , ei.exam_description
                  , ei.graded
                  , ei.grade
                  , ei.totalQuestions
                  , ei.numberCorrect
                  , ei.numberWrong
                  , ei.numberUnanswered
                  , ei.showInHistory
                  , ei.vid
                  , ei.status
                  , ei.created
             FROM   exam_instance ei
             WHERE  ei.exam_instance_id = :eiid
             AND    ei.user_id          = :uid
             AND    ei.showInHistory    = 1'
       , array(':eiid' => $pExamInstanceID, ':uid' => $user->uid)
    );         
    
    foreach ($result as $row) {
      $vExamInfo = array(
          'exam_instance_id' => $row->exam_instance_id
        , 'user_id'          => $row->user_id
        , 'exam_id'          => $row->exam_id
        , 'type'             => $row->type
        , 'exam_title'       => $row->exam_title
        , 'exam_description' => $row->exam_description
        , 'graded'           => $row->graded
        , 'grade'            => $row->grade
        , 'totalQuestions'   => $row->totalQuestions
        , 'numberCorrect'    => $row->numberCorrect
        , 'numberWrong'      => $row->numberWrong
        , 'numberUnanswered' => $row->numberUnanswered
        , 'showInHistory'    => $row->showInHistory
        , 'vid'              => $row->vid
        , 'status'           => $row->status
        , 'created'          => $row->created
        );
    }    
    return $vExamInfo;
  }
  
  /* 
   * Get the completed grade for a given exam_instance_id
   * 
   * @param $pExamInstanceID
   *   The primary key of the exam_instance_id for which we want to retrieve a grdae
   * @result
   *   Returns the grade if the exam hsa been graded, and if the 
   *   requesting user is authorized for access, and if the exam
   *   is still included in exam history.  Otherwise, returns an X.
   */
  static public function getGradeForExamInstanceID($pExamInstanceID) {
    
    global $user;
    $vGrade = 'X'; // Initialize
    // For a given exam_instance_id, returns a grade to an authorized user
    $result = db_query(
            'SELECT grade
             FROM   exam_instance
             WHERE  exam_instance_id = :eiid
             AND    user_id          = :uid
             AND    showInHistory    = 1'
       , array(':eiid' => $pExamInstanceID, ':uid' => $user->uid)
    );         
    
    foreach ($result as $row) {
      $vGrade = $row->grade;
    }    
    return $vGrade;
  } // static public function getGradeForExamInstanceID()
  
  /*
   * Get all information for one question of an exam instance
   * 
   * @param $pExamInstanceID
   * 
   * @param $pQuestionNumber
   * 
   * @return
   *   An array containing all column data for one question in one exam instance
   */
  static public function getOneQuestion($pExamInstanceID, $pQuestionNumber) {
    
    global $user;

    // For a given exam_instance_id, returns a grade to an authorized user
    $result = db_query(
            'SELECT qi.question_instance_id
                  , qi.exam_instance_id
                  , qi.topic_id
                  , qi.topic_instance_id
                  , qi.question_id
                  , qi.question_no
                  , qi.question_text
                  , qi.marked
                  , qi.selected_answer_letters
                  , qi.answeredCorrectly
                  , qi.vid
                  , qi.status
                  , qi.created
             FROM   question_instance qi JOIN exam_instance ei
                    ON qi.exam_instance_id = ei.exam_instance_id
             WHERE  ei.exam_instance_id = :eiid
             AND    ei.user_id          = :uid
             AND    qi.question_no      = :qno
             AND    ei.showInHistory    = 1'
       , array(  ':eiid' => $pExamInstanceID
               , ':uid'  => $user->uid
               , ':qno'  => $pQuestionNumber)
    );         
    
    foreach ($result as $row) {
      $vQuestionInfo = array(
          'question_instance_id'        => $row->question_instance_id
        , 'exam_instance_id'            => $row->exam_instance_id
        , 'topic_id'                    => $row->topic_id
        , 'topic_instance_id'           => $row->topic_instance_id
        , 'question_id'                 => $row->question_id
        , 'question_no'                 => $row->question_no
        , 'question_text'               => $row->question_text
        , 'marked'                      => $row->marked
        , 'selected_answer_letters'     => $row->selected_answer_letters
        , 'answeredCorrectly'           => $row->answeredCorrectly
        , 'vid'                         => $row->vid
        , 'status'                      => $row->status
        , 'created'                     => $row->created
        );
    }        
    
    return $vQuestionInfo;
  } // static public function getOneQuestion()
  
  /*
   * Get full set of answers and each answer's information for one question of an exam instance
   * 
   * @param $pExamInstanceID
   * 
   * @param $pQuestionNumber
   * 
   * @return
   *   A two-dimensional array containing all column data 
   *   for one question's answers in one exam instance
   */
  static public function getOneQuestionsAnswers($pExamInstanceID, $pQuestionNumber) {
    
    global $user;
    $vAnswerInfo = array('');
    
    ExamInstance::storeMessage('$pExamInstanceID = ' . $pExamInstanceID . '.', 'getOneQuestionsAnswers');
    ExamInstance::storeMessage('$pQuestionNumber = ' . $pQuestionNumber . '.', 'getOneQuestionsAnswers');

    // For a given exam_instance_id, returns information about 
    // one question and the answers for that question
    $result = db_query(
            'SELECT qi.question_text
                  , ai.answer_instance_id
                  , ai.question_instance_id
                  , ai.answer_id
                  , ai.answer_number
                  , ai.answer_letter
                  , ai.answer_text
                  , ai.is_correct
                  , ai.rationale
                  , ai.isChosen
                  , ai.vid
                  , ai.status
                  , ai.created
             FROM   question_instance qi 
                      JOIN answer_instance ai
                      ON qi.question_instance_id = ai.question_instance_id 
                      JOIN exam_instance ei
                      ON qi.exam_instance_id = ei.exam_instance_id
             WHERE  ei.exam_instance_id = :eiid
             AND    ei.user_id          = :uid
             AND    qi.question_no      = :qno
             AND    ei.showInHistory    = 1'
       , array(  ':eiid' => $pExamInstanceID
               , ':uid'  => $user->uid
               , ':qno'  => $pQuestionNumber)
    );         
    
    // NOTE:  Do not ever use "answer 0", as that is reserved for the default value by Drupal
    // So start the $vCount at a number no less than 1.
    $vCount = 1;
    foreach ($result as $row) {
      ExamInstance::storeMessage('$vCount = ' . $vCount . '.', 'getOneQuestionsAnswers');
      ExamInstance::storeMessage('row->answer_text' . $row->answer_text . '.', 'getOneQuestionsAnswers');

      array_push($vAnswerInfo, array(
          'answer_instance_id'   => $row->answer_instance_id
        , 'question_instance_id' => $row->question_instance_id
        , 'answer_id'            => $row->answer_id
        , 'answer_number'        => $row->answer_number
        , 'answer_letter'        => $row->answer_letter
        , 'answer_text'          => $row->answer_text
        , 'is_correct'           => $row->is_correct
        , 'rationale'            => $row->rationale
        , 'isChosen'             => $row->isChosen
        , 'vid'                  => $row->vid
        , 'status'               => $row->status
        , 'created'              => $row->created
        ));
      $vCount++;
    }        
    
    return $vAnswerInfo;
  } // static public function getOneQuestion()  
  
  
  /*
   * Stores the value of one question's answer in the database
   * Part of the exam-taking system
   * 
   * @param &$form_state
   * 
   * @return 
   *   null
   */
  static public function setOneQuestionsAnswer($form, &$form_state) {
      
    $vQuestionInstanceID    = $_SESSION['pQuestionInstanceID'];
    $vExamInstanceID        = $_SESSION['exam']['pExamInstanceID'];
    $vCurrentQuestionNumber = $_SESSION['exam']['pCurrentQuestionNumber'];
    
    $vSelectedAnswerInstanceID   = '';
    $vSelectedAnswerLetter       = '';
    $vSelectedAnswerNumber       = '';
    $vSelectedAnswerLetters      = NULL;
    $vTotalNumberOfAnswerOptions = 4;

    // Originally, we checked to see if a response had been provided.
    // However, that no longer works.  Why?  Because now we're allowing
    // checkboxes for multiple-answer questions.  That means the end-user
    // could theoretically de-select all check boxes, leaving no answer.
    // If this is done to a question that had previously been answered,
    // we'll need to NULL out all prior answers.

    $vAnswer = $form_state['values']['pAnswer'];
    // Check to see if $vAnswer is an array, or not.  
    // If $vAnswer is an array, that means the question offered multiple correct answers.
    // If $vAnswer is not an array, that means the question allowed only one correct answer.    
    if (!is_array($vAnswer)) {
      // $vAnswer is not an array. So let's 
      // change it into one to support what will follow.  That way 
      // we only have to have a single set of code that processes arrays,
      // even if the answer comes back as a single variable.
      // The goal of the while loop below is to change something like this:
      //    $vAnswer = 2
      // into this:
      //    $vAnswer[1] = 0
      //    $vAnswer[2] = 2
      //    $vAnswer[3] = 0
      //    $vAnswer[4] = 0
      // So let's do it.
      $vAnswerArray = array();
      $vCounter = 1;
      while ($vCounter <= $vTotalNumberOfAnswerOptions) {
        if ($vAnswer == $vCounter) {
          $vAnswerArray[$vCounter] = $vAnswer;
        } else {
          $vAnswerArray[$vCounter] = 0;
        }
        $vCounter++;
      }
    } else {
      $vAnswerArray = $vAnswer;
    }
    // Determine if any answer was provided.  Normally we shouldn't have to do this,
    // since a NULL value to $vAnswer could've been flagged by now.  However, we 
    // must manually manage NULL responses in the event the test-taker is revisting
    // an answer that was previously responded to, but is not being voided out.
    // This is possible with the multiple-answer option that uses checkboxes.
    // If we just received a response of NULL, we wnat to NULL out any prior answers
    $vCounter  = 1;
    $vResponse = 0;
    while ($vCounter <= $vTotalNumberOfAnswerOptions) {
      $vResponse += $vAnswerArray[$vCounter];
      $vCounter++;
    }
    if ($vResponse == 0 ) {
      $vTheNoAnswerFormat = null; // No answer was provided.  We'll have to NULL out any previously recorded answers.
    } else {
      $vTheNoAnswerFormat = 0; // An answer was provided.  Store the responses using 1's (Y) and 0's (N).
    }
    // Ok, $vAnswerArray contains our answer, regardless of whether the form forwarded an array or not.
    // Note that the answer may or may not contain multiple correct answers.
    $vCounter = 1; // Set $vCounter to 1.  The $vAnswerArray values correspond:  1=1, 2=2, 3=3, and 4=4.
    $vFirstLetterChosen = 1; // Used to decide when to include a comma to separate letters.  
    $vComma = ', '; // 
    while ($vCounter <= $vTotalNumberOfAnswerOptions) {
      // For a multiple choice answer, build a text string showing all selected answers
      // for display in the "show summary" screen
      if ($vAnswerArray[$vCounter] == 0) {
        $vIsChosen = $vTheNoAnswerFormat; // Set $vIisChosen to 0 or NULL for later storage in the database
      } else {
        $vIsChosen = 1;
        if ($vFirstLetterChosen == 1) {
          $vSelectedAnswerLetters .= getLetterForNumber($vCounter);
          $vFirstLetterChosen++;
        } else {
          $vSelectedAnswerLetters .= $vComma . getLetterForNumber($vCounter);
        }
      }
      // Next, update each and every answer_instance record.
      // Why?  Because an answer may have been chosen
      // earlier that is now de-chosen, but we can't 
      // tell right now.  So let's set all values 
      // to null except those that are chosen.
      $result = db_update('answer_instance')
          -> fields(array(
               'isChosen' => $vIsChosen // 0 if some other answer was selected, NULL if no answer to any anwer option was provided.
          ))
          ->condition('question_instance_id', $vQuestionInstanceID)
          ->condition('answer_number',        $vCounter)
          ->execute();    
      $vCounter++;
    }
    
    //ExamInstance::storeMessage('.' . $vSelectedAnswerLetters . '.', 'exam.class.inc - setOneQuestionsAnswer');
    
    // Build query to determine if the question has been
    // answered correctly or not.
    $vUpdateStatement = 
      'UPDATE question_instance
         SET marked                  = :mark 
           , selected_answer_letters = :ltrs
       WHERE  question_instance_id   = :qid';
    $num_updated = db_query($vUpdateStatement
                    , array(  ':mark' => $form_state['values']['pMark']
                            , ':ltrs' => $vSelectedAnswerLetters
                            , ':qid'  => $vQuestionInstanceID));
       
  } // static public function setOneQuestionsAnswer()

  
  /*
   * examCleanup. Cleans up session variables for an exam session.
   * 
   * @param 
   *   None.  
   * 
   * @return
   *   null.
   * 
   */
  static public function examCleanup() {

    // Clear session variables
    unset($_SESSION['exam']['pExamID']);
    unset($_SESSION['exam']['pExamInitialize']);
    unset($_SESSION['exam']['pCurrentQuestionNumber']);
    unset($_SESSION['exam']['pExamTitleURL']);
    unset($_SESSION['exam']['pExamInstanceID']);
    unset($_SESSION['exam']['pButtonClicked']);
    unset($_SESSION['exam']['pNextButtonClick']);

    return null;
    
  } // statuc public function examCleanup()
  
  //
  // Constructor
  //
  // Since PHP doesn't allow overloaded methods, I've 
  // created a workaround.
  //
  // VERSION 1: ($vExamID) - for taking new exam
  // VERSION 2: (0, $vExamInstanceID) - for continuing 
  //            previously started exam that was 
  //            interrupted for whatever reason
  //
  function __construct($pExamID = 0, $pExamInstanceID = 0) {

    ExamInstance::storeMessage('1 and the pExamInstanceID is ' . $pExamInstanceID . '!', 'constructor.');
    ExamInstance::storeMessage('SKERE = pExamID = ' . $pExamID . '.', 'constructor');
    ExamInstance::storeMessage('SKERE = pExamInstanceID = ' . $pExamInstanceID . '.', 'constructor');
    if ($pExamInstanceID == 0) {
      ExamInstance::storeMessage('4', 'constructor.');
      $vExamInstanceID = $this->populateInstanceTables($pExamID);
      $this->loadExamObject($vExamInstanceID);
      return $this;
      //return $this->loadNewExamObject($pExamID);
    } else {
      ExamInstance::storeMessage('8', 'constructor.');
      return $this->loadExamObject($pExamInstanceID);
    }
    ExamInstance::storeMessage('10', 'constructor.');
    } // function __construct     
  
  /*
   * This function copies a complete exam
   * from the master tables:
   *    - exam
   *    - topic
   *    - question
   *    - answer
   * to the corresponding instance tables:
   *    - exam_instance
   *    - topic_instance
   *    - question
   *    - answer
   * It returns the exam_instance_id generated
   * by inserting the new row in exam_instance
   *
   */

  static public function populateInstanceTables($pExamID) {

    global $user;

    // Create a counter to be used in the calculation of each question number
    $vQuestionCounter = 0;

    // ************************************************
    // * First, Copy row (just one) from exam to exam_instance 
    // ************************************************

    // Step 1a of 4
    // There should only be one row from the exam table
    $resultExam = db_query(
         'SELECT   e.exam_id
                 , e.type
                 , e.exam_title
                 , e.exam_description
                 , e.vid
                 , e.status
                 , count(q.question_id) totalQuestions
          FROM     exam e LEFT JOIN question q 
          ON      (e.exam_id = q.exam_id)
          WHERE    e.exam_id = :eid
          GROUP BY e.exam_id
                 , e.type
                 , e.exam_title
                 , e.exam_description
                 , e.vid
                 , e.status',
          array(':eid' => $pExamID)
          );


    foreach ($resultExam as $recordExam) {

      // Step 1b of 4
      // Insert the exam record into the exam_instance table.  
      // Be sure to capture the newly created primary key value, which
      // is the exam_instance_id.  We need that later in this function.
      $eiid = db_insert('exam_instance')
          ->fields(array(
              'exam_id'          => $recordExam->exam_id
            , 'type'             => $recordExam->type
            , 'exam_title'       => $recordExam->exam_title
            , 'exam_description' => $recordExam->exam_description
            , 'graded'           => 0
            , 'totalQuestions'   => $recordExam->totalQuestions
            , 'vid'              => $recordExam->vid         
            , 'status'           => $recordExam->status
            , 'created'          => REQUEST_TIME
            , 'user_id'          => $user->uid
          ))
          ->execute();
      
      ExamInstance::storeMessage('$eiid =  . $eiid' . '.', 'populateInstanceTable');

    } // $recordExam

    // Step 2a of 4
    // Next, get the master data on the topics for this exam
    $resultTopic = db_query(
        'SELECT t.topic_id
              , t.topic_title
              , t.for_more_info_see
              , t.vid
              , t.status
         FROM   topic t
         WHERE  t.exam_id = :eid',
         array(':eid' => $pExamID)
        );

    foreach ($resultTopic as $recordTopic) {

      // Step 2b of 4
      // Use the master data to create
      // a set of rows in the topic_instance table.
      // Be sure to capture the newly created topic_instance_id value        
      $tiid = db_insert('topic_instance')
          ->fields(array(
              'exam_instance_id'  => $eiid
            , 'topic_id'          => $recordTopic->topic_id
            , 'exam_instance_id'  => $eiid
            , 'topic_title'       => $recordTopic->topic_title
            , 'for_more_info_see' => $recordTopic->for_more_info_see
            , 'vid'               => $recordTopic->vid
            , 'status'            => $recordTopic->status
            , 'created'           => REQUEST_TIME
          ))
          ->execute();  

    } // $recordTopic
         
    // Step 3a of 4
    //$vTotalNumberOfQuestions = $_SESSION['exam']['pTotalNumberOfQuestions'];
    //print "Skere - vTotalNumberOfQuestions = " . $vTotalNumberOfQuestions . "<br>";
    $resultQuestion = db_query(
        
        'SELECT   q.topic_id
                , ti.topic_instance_id 
                , q.question_id
                , q.question_text
                , q.explanation_if_wrong
                , q.vid
                , q.status
         FROM     question AS q JOIN topic_instance AS ti
                  ON q.topic_id = ti.topic_id
         WHERE    q.exam_id           = :eid
           AND    ti.exam_instance_id = :eiid
         ORDER BY RAND()',
         array(':eid' => $pExamID, ':eiid' => $eiid)
        );
    
    foreach ($resultQuestion as $recordQuestion) {
   
      // Step 3b of 4
      // Increment the question number
      $vQuestionCounter++;

      // Initialize ei_answerSet          
      //$this->ei_answerSet = array('0');

      // Insert the question record.  Capture the newly created primary key value.
          
      $qiid = db_insert('question_instance')
          ->fields(array(
              'exam_instance_id'     => $eiid
            , 'topic_id'             => $recordQuestion->topic_id 
            , 'topic_instance_id'    => $recordQuestion->topic_instance_id
            , 'question_id'          => $recordQuestion->question_id
            , 'question_no'          => $vQuestionCounter
            , 'question_text'        => $recordQuestion->question_text
            , 'explanation_if_wrong' => $recordQuestion->explanation_if_wrong
            , 'vid'                  => $recordQuestion->vid
            , 'status'               => $recordQuestion->status
            , 'created'              => REQUEST_TIME
          ))
          ->execute();

      // Step 4a of 4                  
      $resultAnswer = db_query(
          'SELECT a.answer_id
                , a.answer_text
                , a.is_correct
                , a.rationale
                , a.vid
                , a.status
           FROM   answer a
           WHERE  a.question_id = :qid',
          array(':qid' => $recordQuestion->question_id)
          );

      // Step 4b of 4
      // Create a counter.  Primary purpose is to walk through
      // the array of letters ('A', 'B', etc.) to assign to answers.
      $vAnswerCounter = 0;
      foreach ($resultAnswer as $recordAnswer) {
        $vAnswerCounter++;            

        // Insert the answer record into the answer_instance table.   

        $aiid = db_insert('answer_instance')
            ->fields(array(
              'question_instance_id' => $qiid
            , 'answer_id'            => $recordAnswer->answer_id
            , 'answer_number'        => $vAnswerCounter
            , 'answer_letter'        => ExamInstance::$sRangeLetters[$vAnswerCounter]
            , 'answer_text'          => $recordAnswer->answer_text
            , 'is_correct'           => $recordAnswer->is_correct
            , 'rationale'            => $recordAnswer->rationale
            , 'vid'                  => $recordAnswer->vid
            , 'status'               => $recordAnswer->status
            , 'created'              => REQUEST_TIME
           ))
           ->execute();
            
      } // $recordAnswer

    } // $recordQuestion

    return $eiid; // this is the newly created exam_instance_id value

  } // function populateInstanceTables($pExamID)
   
/*
 * This function loads an ExamInstance
 * object from the following instance tables:
 *    - exam_instance
 *    - topic_instance
 *    - question
 *    - answer
 * It returns the object.
 * This is performed for one of two reasons:
 *    - to initiate an exam
 *    - to continue a previous exam 
 *      session that was interrupted 
 *      for whatever reason
 *
 */

 function loadExamObject($pExamInstanceID) {

    ExamInstance::storeMessage('Just inside', 'loadExamObject');
   
    global $user;

    //$vTotalNumberOfQuestions = $_SESSION['exam']['pTotalNumberOfQuestions'];
    //ExamInstance::storeMessage('Skere - vTotalNumberOfQuestions = ' . $vTotalNumberOfQuestions . '!', 'loadExamObject');
    
    // Step 1a of 4:  Get data from exam_instance
    // There will only be one row from the exam table
    $resultExam = db_query(
         'SELECT   ei.exam_id
                 , ei.type
                 , ei.exam_title
                 , ei.exam_description
                 , ei.vid
                 , ei.status
                 , ei.totalQuestions
          FROM     exam_instance ei 
          WHERE    ei.exam_instance_id = :eiid',
          array(':eiid' => $pExamInstanceID)
          );

    //ExamInstance::storeMessage('SKERE MARK 17 $pExamInstanceID = ' . $pExamInstanceID . '!', 'loadExamObject');
    
    foreach ($resultExam as $recordExam) {

      // Step 1b of 4: Build array for data from exam_instance
      // Building the exam object
      // Part 1: Capture the exam info into an array
      // Note:  there should only be one row of data for this,
      // thus, an array of one dimension
      $this->ei_exam = array(
          'exam_instance_id' => $pExamInstanceID, 
          'exam_id'          => $recordExam->exam_id,
          'exam_title'       => $recordExam->exam_title,
          'type'             => $recordExam->type,
          'exam_description' => $recordExam->exam_description,
          'graded'           => 0,
          'grade'            => null,
          'totalQuestions'   => $recordExam->totalQuestions,
          'numberCorrect'    => null,
          'numberWrong'      => null,
          'numberUnanswered' => null,
          'vid'              => $recordExam->vid,
          'status'           => $recordExam->status,
        );

      //ExamInstance::storeMessage('SKERE = $recordExam->totalQuestions = ' . $recordExam->totalQuestions . '!', 'loadExamObject');

      // Step 2a of 4: Get data from topic_instance
      // Next, get topics for this exam
      $resultTopic = db_query(
          'SELECT ti.topic_id
                , ti.topic_instance_id
                , ti.topic_title
                , ti.vid
                , ti.status
           FROM   topic_instance ti
           WHERE  ti.exam_instance_id = :eiid',
           array(':eiid' => $pExamInstanceID)
          );

      foreach ($resultTopic as $recordTopic) {

        //ExamInstance::storeMessage('SKERE New Topic ID ' . $recordTopic->topic_id . '!', 'loadExamObject');
        
        // Step 2b of 4: Build array for data from topic_instance
        // Building the exam object
        // Part 2: Store the topic info into an array   
        // Be sure to include the newly created
        // exam_instance_id, stored in the $eiid variable,
        // and the newly created topic_instance_id variable,
        // stored in $tiid.
        // 
 
        array_push($this->ei_topic, 
            array(  'topic_id'          => $recordTopic->topic_id
                  , 'topic_instance_id' => $recordTopic->topic_instance_id
                  , 'exam_instance_id'  => $pExamInstanceID
                  , 'topic_title'       => $recordTopic->topic_title
                  , 'totalQuestions'    => null
                  , 'numberCorrect'     => null
                  , 'numberWrong'       => null
                  , 'numberUnanswered'  => null            
                 )
            ); 

        // Step 3a of 4: Get data from question_instance
        $resultQuestion = db_query(
            'SELECT qi.question_instance_id
                  , qi.question_id
                  , qi.topic_instance_id
                  , qi.topic_id
                  , qi.question_no
                  , qi.question_text
                  , qi.marked
                  , qi.selected_answer_letters
                  , qi.explanation_if_wrong
                  , qi.vid
                  , qi.status
             FROM   question_instance qi
             WHERE  qi.exam_instance_id  = :eiid
               AND  qi.topic_instance_id = :tiid',
             array(':eiid' => $pExamInstanceID, ':tiid' => $recordTopic->topic_instance_id)
            );

        foreach ($resultQuestion as $recordQuestion) {


          //ExamInstance::storeMessage('SKERE = inside question loop.', 'loadExamObject');
    
          // Increment the question number
          // $vQuestionCounter++;


          // Initialize ei_answerSet          
          $this->ei_answerSet = array('0');

          // Step 4a of 4: Get data from answer_instance
          $resultAnswer = db_query(
              'SELECT ai.answer_instance_id
                    , ai.answer_id
                    , ai.answer_text
                    , ai.answer_letter
                    , ai.is_correct
                    , ai.rationale
                    , ai.vid
                    , ai.status
               FROM   answer_instance ai
               WHERE  ai.question_instance_id = :qiid',
              array(':qiid' => $recordQuestion->question_instance_id)
              );


          foreach ($resultAnswer as $recordAnswer) {       

            //ExamInstance::storeMessage('SKERE = inside answer loop.', 'loadExamObject');
          
            // Step 4b of 4: Build array for data from answer_instance
            // Create a member variable that consists of an array of answers
            // for a single question on the exam.  
            array_push($this->ei_answerSet, 
                array(  'answer_id'          => $recordAnswer->answer_id 
                      , 'answer_instance_id' => $recordAnswer->answer_instance_id
                      , 'answer_letter'      => $recordAnswer->answer_letter
                      , 'answer_text'        => $recordAnswer->answer_text
                      , 'is_correct'         => $recordAnswer->is_correct
                      , 'rationale'          => $recordAnswer->rationale
                     )
                );     

          } // $recordAnswer

          // Step 3b of 4: Build array for data from question_instance
          // Create a member variable that consists of an array of questions
          // for the exam, and include a single array of answers ($ei_answerSet)
          // which is the set of answers that pertain to just this one question
          array_push($this->ei_qa, 
              array(  'question_id'                 => $recordQuestion->question_id 
                    , 'question_instance_id'        => $recordQuestion->question_instance_id
                    , 'topic_id'                    => $recordQuestion->topic_id
                    , 'topic_instance_id'           => $recordQuestion->topic_instance_id
                    , 'question_no'                 => $recordQuestion->question_no
                    , 'question_text'               => $recordQuestion->question_text
                    , 'theAnswers'                  => $this->ei_answerSet
                    , 'marked'                      => $recordQuestion->marked
                    , 'selected_answer_letters'     => $recordQuestion->selected_answer_letters
                    , 'answeredCorrectly'           => null // null means "unanswered".
                    , 'explanation_if_wrong'        => $recordQuestion->explanation_if_wrong
                   )
              );                                 

        } // $recordQuestion


      } // $recordTopic


    } // $recordExam

    ExamInstance::storeMessage('SKERE = Returning loaded object.', 'loadExamObject');
    
    return $this;

  } // function loadExamObject($pExamInstanceID)  
  
}

?>
