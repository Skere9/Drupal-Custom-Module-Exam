<?php

/**
 * Display an exam for a student.
 * 
 * This is the initial page for a given exam.
 * 
 * From this page, a student can click to 
 * start taking the exam.
 *
 * 
 * @param $view_mode
 *   The view mode we want to display.
 */
function exam_welcome($pExamID) {
  
  ExamInstance::storeMessage('SKERE - Here.', 'exam_welcome');
  
  jquery_loadinganimation_install();
  _jquery_loadinganimation_get_settings();
  jquery_loadinganimation_init();
       
  // Initialize the $vContent text variable collecting the HTML page.
  // This prevents us from having to figure out which 
  // line consists of the first assignment to the variable.
  // In other words, each assignment to this variable that follows
  // will append to it, instead of initializing it.
  $vContent = ''; 
  
  $result = db_query(
      'SELECT   e.exam_title 
              , COUNT(q.question_id) as tot_q 
       FROM     exam as e LEFT JOIN question as q 
                ON e.exam_id = q.exam_id 
       WHERE    e.exam_id = :eid 
       GROUP BY e.exam_title '
      , array(':eid' => $pExamID)); 
    
  if ($result->rowCount() == 0){
    $vContent .= '<p>There is no such exam stored in the system.</p>';
  }
  else { 
    while ($row = $result->fetchAssoc()) {
      // Edit this - there should only be one iteration of this loop
      $vExamTitleURL = ExamInstance::getExamTitleForURL($row['exam_title']);
      $vContent .= '<p>Welcome to the exam!</p>';
      $vContent .= '<p>Exam Title: ' . $row['exam_title'] . '</p>';
      if ($row['tot_q'] == '1') {
        $vQtext = 'question';
      } else {
        $vQtext = 'questions';
      }   
  
      $vContent .= '<p>This exam consists of ' . 
                 $row['tot_q'] . ' ' . $vQtext . '.</p>';
      $vExamQuestionNumber = 1; // Start at question # 1
      $vContent .= '<p><a id="loadExam" href="?q=exam/take/' . 
                 $vExamTitleURL
                 . '/' .  $vExamQuestionNumber . '">Click to start.</a></p>';
    }
    // Note:  some parameters for the exam session are initialized
    // here.  Others are initialized in the next function.
    $_SESSION['exam']['pExamID'] = $pExamID;
    $_SESSION['exam']['pExamInitialize'] = '1'; // A flag to initialize the exam object
    $_SESSION['exam']['pCurrentQuestionNumber'] = '1';
    $_SESSION['exam']['pExamTitleURL'] = $vExamTitleURL;
  } 
  return t($vContent);
  
} // function exam_welcome()


/**
 * Display an exam for a student who is returning to continue
 * taking an exam already initiated in an earlier session,
 * but left uncompleted.
 * 
 * This is the initial page for continuing an exam in a new session.
 * 
 * From this page, a student can click to 
 * continue taking the exam.
 *
 * @param $view_mode
 *   The view mode we want to display.
 */
function exam_welcome_back($form, &$form_state, $pExamInstanceID) {

  global $user;
  
  ExamInstance::storeMessage('$pExamInstanceID = ' . $pExamInstanceID, 'exam_welcome_back');
  $vContent = '';
  $vExamTitle = '';
  
  $result = db_query(
    'SELECT ei.graded
          , ei.exam_title
          , ei.totalQuestions
          , COUNT(qi.selected_answer_letters) numberAnswered          
     FROM     exam_instance AS ei JOIN question_instance AS qi
              ON ei.exam_instance_id = qi.exam_instance_id
     WHERE ei.exam_instance_id = :eiid
       AND ei.user_id          = :uid
     GROUP BY ei.graded
            , ei.exam_title
            , ei.totalQuestions'      
      , array(':eiid' => $pExamInstanceID, ':uid' => $user->uid)); 

  // Make sure that there is an exam matching the pExamInstanceID
  if ($result->rowCount() == 0){
    $vContent .= '<p>There is no such exam stored in the system.</p>';
    $form['welcomeMessage'] = array(
      '#markup' => t('<p>' . $vContent . '<p>')
    );    
  }
  else { 
    while ($row = $result->fetchAssoc()) {
      // (SKERE Note: this while loop should be replaced 
      // since this should not be performed more than once,
      // the query should only return a single row.)
      // Confirm that this is a request to continue
      // with an exam that is not yet graded.
      // if graded <> 0 then return oops;  
      if  ($row['graded'] <> 0) {
       // This exam has already been graded.  It cannot be "continued".
        $form = exam_take_oops($form, $form_state);
        return $form;    
      }    
      // We've confirmed the existence of the exam, and 
      // confirmed that it has not yet been graded.  Proceed.

      $vContent .= '<p>Welcome back to your exam!</p>';
      $vContent .= '<p>Exam Title: ' . $row['exam_title'] . '</p>';
            
      // We'll need the vExamTitle and vTotalNumberOfQuestions later in 
      // exam_continue_preparation.  Capture the variables here, 
      // store them in the session variables at the end.
      $vTotalNumberOfQuestions = $row['totalQuestions'];
      $vExamTitle = $row['exam_title'];
  
      $vContent .= '<p>'
        . 'Total number of questions: ' . $row['totalQuestions'] . '.<br/>'
        . 'Number answered so far: ' . $row['numberAnswered'] . '.<br/>'
        . 'Unanswered questions remaining: ' . ($row['totalQuestions'] - $row['numberAnswered']) 
        . '.</p>'; 
    }
    
    $form['welcomeMessage'] = array(
      '#markup' => t('<p>' . $vContent . '<p>')
    );    
        
    $form['actions']['pButtonSummary'] = array(
      '#type'   => 'submit',
      '#value'  => t('Continue with Exam - Show Summary'),
      '#name'   => 'Summary',
      '#submit' => array('exam_continue_preparation')
    );
  
    // Note:  some parameters for the exam session are initialized
    // here.  Others are initialized in the next function.
    $_SESSION['exam']['pExamTitleURL']     = ExamInstance::getExamTitleForURL($vExamTitle);     
    $_SESSION['exam']['pExamInstanceID']   = $pExamInstanceID;
    $_SESSION['exam']['pExamInitialize']   = '0'; // Do NOT "initialize" a new exam.  Continue with existing exam.
  
    ExamInstance::storeMessage('pExamTitleURL='   . $_SESSION['exam']['pExamTitleURL']   . '.' , 'exam_welcome_back');
    ExamInstance::storeMessage('pExamInstanceID=' . $_SESSION['exam']['pExamInstanceID'] . '.' , 'exam_welcome_back');
    ExamInstance::storeMessage('pExamInitialize=' . $_SESSION['exam']['pExamInitialize'] . '.' , 'exam_welcome_back');
    
  } 
  return $form;
} // function exam_welcome_back()

function exam_continue_preparation($form, &$form_state) {
  
  ExamInstance::storeMessage('1' . $_SESSION['exam']['pExamTitleURL'], 'exam_continue_preparation.');
  ExamInstance::storeMessage('1.5 - The pExamInstanceID is' . $_SESSION['exam']['pExamInstanceID'] . '!', 'exam_continue_preparation');
  
  $vExamTitleURL = $_SESSION['exam']['pExamTitleURL'];
  ExamInstance::storeMessage('3 - $vExamTitleURL=' . $vExamTitleURL, 'exam_continue_preparation.');
  $form_state['redirect'] = array('exam/take/' . $vExamTitleURL . '/summary');
} // function exam_continue_preparation()

/*
 * Display a list of exams this user has already taken.
 * 
 */
function exam_history_list() {
  
  global $user;
  
  ExamInstance::storeMessage('Just before call to table sorter', 'exam_history_list');
  ExamInstance::addJQueryTablesorter();
  ExamInstance::storeMessage('Just after call to table sorter', 'exam_history_list');
  
  // This variable contains the number of columns
  // in the history list table.  This is required
  // to support the colspan option in the first row
  // when there are no exams in the history yet.
  $vNumberOfColumns = '10';
  
  $vContent = '';
  
  // Note the unusual manner in which we calculate numberUnanswered.
  // The reason is to make sure we count the number unanswered for 
  // both finished exams, which is stored in the question_instance
  // table, and unfinished exams, which is not stored in question_instance.
  $result = db_query(
    'SELECT   ei.exam_instance_id
            , ei.exam_id
            , ei.type
            , ei.exam_title
            , ei.grade
            , ei.graded
            , ei.totalQuestions
            , ei.numberCorrect
            , ei.numberWrong
            , (SELECT   COUNT(*)
               FROM     question_instance AS qi
               WHERE    qi.selected_answer_letters IS NULL 
                 AND    qi.exam_instance_id = ei.exam_instance_id) numberUnanswered
            , from_unixtime(ei.created) date_completed
     FROM     exam_instance AS ei
     WHERE    ei.user_id = :uid
       AND    ei.showInHistory = 1 
     ORDER BY ei.created DESC',
      array(':uid' => $user->uid)
      );     
          
  $vContent = "<hr/>";
  
  $vContent .= '<table id="theTable" class="tablesorter" cellspacing="1">'; // id and class required for tablesorter function
  
  $vContent .= '<thead>';
  $vContent .= '<tr>';
  $vContent .= '<th width=160>Exam title</th>';
  $vContent .= '<th>Grade</th>';
  $vContent .= '<th>Status</th>';
  $vContent .= '<th>Personalized<br>Report</th>';
  $vContent .= '<th>Questions</th>';
  $vContent .= '<th>Correct</th>';
  $vContent .= '<th>Incorrect</th>';
  $vContent .= '<th>Unanswered</th>';
  $vContent .= '<th width=40>Started</th>';
  $vContent .= '<th>Delete</th>';
  $vContent .= '</tr>';
  $vContent .= '</thead>';
  
  $vContent .= '<tbody>';
  
  if ($result->rowCount() == 0) {
    $vContent .= '<tr><td colspan=' 
               . $vNumberOfColumns 
               . ' align=center>You have not yet completed any exams.<br>'
               . l('Click here to see a list of available exams.','exam/list')
               . '</td></tr>';
  }
  
  $vImageInfoIcon = array(
      'path'  => 'sites/all/modules/exam/images/icons/InfoMid.jpg'
    , 'alt'   => 'Information about the exam'
    , 'title' => ''
    , 'attributes' => array('border' => '0')
  );
  $vImageContinueIcon = array(
      'path'  => 'sites/all/modules/exam/images/icons/ContinueMid.jpg'
    , 'alt'   => 'Continue with the exam'
    , 'title' => ''      
    , 'attributes' => array('border' => '0')
  );
  $vImageDeleteIcon = array(
      'path'  => 'sites/all/modules/exam/images/icons/DeleteIconMid.jpg'
    , 'alt'   => 'Delete the exam'
    , 'title' => ''      
    , 'attributes' => array('border' => '0')
  );
  
  foreach ($result as $row){    
    $vContent .= '<tr>';
    $vContent .= '<td class=exam_icon>' . $row->exam_title     . '</td>';
    $vContent .= '<td class=exam_icon>' . $row->grade . '</td>'; 
    $vContent .= '<td class=exam_icon>';
    if (!is_null($row->grade)) {
      $vContent .= '<a href=?q=exam/record/' . $row->exam_instance_id 
          . ' alt="See details">' . theme('image', $vImageInfoIcon) . '</a></td>';
    } else {
      $vContent .= '<a href=?q=exam/continue/' 
                 . ExamInstance::getExamTitleForURL($row->exam_title) 
                 . '/'. $row->exam_instance_id 
                 . ' alt="Continue taking exam">'
                 . theme('image', $vImageContinueIcon) . '</a></td>';
    }
    $vContent .= '<td class=exam_icon>';
    if (!is_null($row->grade)) {
      $vContent .= '<a href=?q=exam/report/' . $row->exam_instance_id 
          . ' alt="See personalized report">' . 'REPORT' . '</a></td>';
    } else {
      $vContent .= '</td>';
    }    
    $vContent .= '<td class=exam_icon>' . $row->totalQuestions . '</td>';
    $vContent .= '<td class=exam_icon>' . $row->numberCorrect  . '</td>';
    $vContent .= '<td class=exam_icon>' . $row->numberWrong . '</td>';
    $vContent .= '<td class=exam_icon>' . $row->numberUnanswered . '</td>';
    $vContent .= '<td class=exam_icon>' . $row->date_completed . '</td>';
    //$vContent .= '<td class=exam_icon>' . l('X','exam/history/removeOne/' . $row->exam_instance_id) . '</td>';
    $vContent .= '<td class=exam_icon>' . '<a href=?q=exam/history/removeOne/' . $row->exam_instance_id . ' alt="Delete the exam from your history">' . theme('image', $vImageDeleteIcon) . '</a></td>';
    //$vContent .= '<td class=exam_icon>' . '<a class=tooltip href=exam/history/removeOne/' . $row->exam_instance_id . '><img src=sites/all/modules/exam/images/icons/DeleteIconMid.jpg border=0>' . '<span class="custom warning"><img src=images/icons/Warning.jpg height=48 width=48/><em>Warning</em>This action will delete your exam from your history.</span>' . '</a>' . '</td>';
    $vContent .= '</tr>';  
  }
  
  $vContent .= '</tbody>';
  $vContent .= '</table>';
  
  return t($vContent);

} // function exam_history_list()

function exam_history_list_removeOne($form, &$form_state, $exam_instance_id) {
  
  global $user;
  
  $vContent = '';

  $result = db_query(
     'SELECT   e.exam_id
             , e.type
             , e.exam_title
             , e.graded
             , e.totalQuestions
             , e.numberCorrect
             , (round(e.numberCorrect/e.totalQuestions,2))*100 grade
             , from_unixtime(e.created) date_completed
      FROM     exam_instance AS e
      WHERE    e.exam_instance_id = :eiid 
        AND    e.user_id          = :uid' 
    , array(':eiid' => $exam_instance_id, ':uid' => $user->uid)
    );
  
  if ($result->rowCount() == 0){
    
    $vContent .= '<p>You do not have privileges to delete this particular exam.</p>';
    $form['theMessage'] = array(
      '#markup' => t($vContent)
    );
   
  } else {
    $vContent .= '<p>You are about to remove the following exam from your exam history:</p>';
  
    $vContent .= '<table>';
    foreach ($result as $row){
      $vContent .= '<tr><th width=140>Exam title</th><td>' . $row->exam_title     . '</td></tr>'
                 . '<tr><th>Grade</th><td>'                . $row->grade          . '</td></tr>'
                 . '<tr><th>Total # Questions</th><td>'    . $row->totalQuestions . '</td></tr>'
                 . '<tr><th>Number Correct</th><td>'       . $row->numberCorrect  . '</td></tr>'
                 . '<tr><th>Date Taken</th><td>'           . $row->date_completed . '</td></tr>';
    }
    $vContent .= '</table>';
  
    $vContent .= '<p>Are you sure you want to do this?</p>';
  
    $form['theMessage'] = array(
      '#markup' => t($vContent)
    );
  
    $form['pExamInstanceId'] = array(
      '#type' => 'hidden',
      '#value' => $exam_instance_id
    );

    $form['actions']['pButtonDelete'] = array(
      '#type'   => 'submit',
      '#value'  => t('Yes, delete it'),
      '#name'   => 'Delete_Confirmed',
      '#submit' => array('exam_history_list_removeOne_confirmed'
                 )
    );
  
    $form['pCancelDelete'] = array(
      '#markup' => l(t('Cancel'),'exam/history')
    ); 
  } 
  return $form;
}  // function exam_history_list_removeOne()

function exam_history_list_removeOne_confirmed($form, &$form_state) {
  
  global $user;
 
  print '<p>This is the exam_instance_id:' . $form_state['values']['pExamInstanceId'] . '</p>';

  $result = db_update('exam_instance')
    -> fields(array(
      'showInHistory' => 0
      ))
    ->condition('user_id', $user->uid)
    ->condition('exam_instance_id', $form_state['values']['pExamInstanceId'])
    ->execute();
  
  $form_state['redirect'] = array('exam/history');

  return null;  

} // function exam_history_list_removeOne_confirmed()

/*
 * function
 * 
 * @param $form
 * 
 * @param $form_state
 * 
 * @param $pExamID
 * 
 * @param $pQNO
 * 
 * @return 
 * 
 */

function exam_take_form($form, &$form_state, $pExamID, $pQNO=0){
  
  $vCurrentQuestionNumber = 0; // This should be properly defined later
  $vDefaultAnswerNumbers = array(); // This is to fill the form with previously provided answers
  $vDefaultValue = null; // Used to hold any previously provided answer for a single-answer question
  
  //Prepare this function for iterative use
  //  - ensure the $vExamInstance is only created the first time
  //  - pass back into this function a value for the question number
  //  - capture student answers
  //print 'exam = ' . $pExamID;
  //
  // $_SESSION['exam']['pExamInitialize'] = 1:  Initiate a session for a new exam
  // $_SESSION['exam']['pExamContinuation'] = 2:  Initiate a session for an existing exam
  // 
  // Capture SESSION variables, if they exist.  This is not necessary,
  // but if we wait and try to reference the session variables later,
  // we have to first check if the variable exists (isset), and 
  // that's so annoying. It detracts from the code's readability.
  if (isset($_SESSION['exam']['pExamInitialize'])) { 
    $pExamInitialize = $_SESSION['exam']['pExamInitialize'];
    // Unset the session variable pExamInitiative so that the exam 
    // is not initialized again unless a new exam is launched.    
    unset($_SESSION['exam']['pExamInitialize']);
  } else {
    $pExamInitialize = 0;
  }
  
  if ($pExamInitialize == '1') {
    // Initialize the page advance controls
    exam_take_page_advance_init();
    // Initialize the exam - create an exam instance in the database
    // and generate a new exam_instance_id.  Capture the exam_instance_id 
    // in a variable, we'll need this.
    $vExamInstanceID = ExamInstance::populateInstanceTables($pExamID);   
    $_SESSION['exam']['pExamInstanceID'] = $vExamInstanceID;
  } else {
    // Obtain the already-established exam_instance_id
    $vExamInstanceID = $_SESSION['exam']['pExamInstanceID'];
  }
 
  ExamInstance::storeMessage('$vExamInstanceID = ' . $vExamInstanceID, 'exam_take_form');
  
  // SKERE777 add logic for what happens with bogus incoming values for pExamInstanceID
  
  // Get basic exam data
  $vExamInfo = ExamInstance::getInfoForExamInstanceID($vExamInstanceID);

  //
  // Confirm that the incoming request is asking for a valid question number
  // pQNO is "Parameter for a Question Number".  All incoming parameters should
  // start with a "p".  But that may not be fully enforced yet.
  //
  if (($pQNO > 0) and ($pQNO <= $vExamInfo['totalQuestions'])) { 
    $vCurrentQuestionNumber = $pQNO;
    $_SESSION['exam']['pCurrentQuestionNumber'] = $vCurrentQuestionNumber;
  } else {
    // 
    // SKERE - WORK ON THIS
    // Display an error message warning that the 
    // incoming request is for a non-existent question number
    ExamInstance::storeMessage('ERROR: Invalid question number ' . $pQNO . ' ***', 'exam_take_form');
    $form['pQuestionNumber'] = array(
      '#markup' => t('Invalid question number ' . $pQNO 
          . ' of ' 
          . $vExamInfo['totalQuestions'] . '</h2></p>')
    );    
    return $form;
  }
  
  // Get the array of answers for the current question
  $vTheQuestion = ExamInstance::getOneQuestion($vExamInstanceID, $vCurrentQuestionNumber);
  $_SESSION['pQuestionInstanceID'] = $vTheQuestion['question_instance_id']; // needed to store answer later
  ExamInstance::storeMessage('$vTheQuestion - text is "' . $vTheQuestion['question_text'] . '"', 'exam_take_form');
  $vTheAnswers  = ExamInstance::getOneQuestionsAnswers($vExamInstanceID, $vCurrentQuestionNumber);
  $vNumberOfAnswers = count($vTheAnswers)-1; // Account for the first initialized blank row.  Looping starts at 0 and needs to iterate four times, so 0, 1, 2, 3.  Three is the end.
  
  ExamInstance::storeMessage('$vNumberOfAnswers = ' . $vNumberOfAnswers, 'exam_take_form');
    
  $form['pQuestionNumber'] = array(
    '#markup' => t('<p><h2>Question #' 
        . $vCurrentQuestionNumber . ' of ' 
        . $vExamInfo['totalQuestions'] . '</h2></p>')
  );
  
  $form['pMark'] = array(
    '#type' => 'checkbox',
    '#title' => t('Mark'),
    '#default_value' => $vTheQuestion['marked'] // Skere - check this
  );
     
  // Create and initialize a counter 
  // to support the creation of 
  // an array of answers for the current question
  // to be used in the form['pAnswer'] object 
  $vCounter = 1;
  // Initialize a variable to count the number of answers that are correct
  $vNumberOfCorrectAnswers = 0;  
  while ($vCounter <= $vNumberOfAnswers) {
    // Extract the answer_instance_id value
    //$vAnswerInstanceID = $vTheAnswers[$vCounter]['answer_instance_id'];
    // Build the answer string to be displayed 
    $vAnswerString = '<b>' . $vTheAnswers[$vCounter]['answer_letter'] . '</b>: ' .
      $vTheAnswers[$vCounter]['answer_text'];
    // Combine the answer letter and the string and build the array
    // REPLACE THIS WITH THE NEXT LINE: $vAnswerArray[$vAnswerInstanceID] = $vAnswerString;
    $vAnswerArray[$vCounter] = $vAnswerString;
    // While we're at it, count the number of answers that are "correct", 
    // so we can tell if this question has a single correct answer, or multiples.
    // Since a 0 indicates the answer is not correct, and a 1 indicates it is correct,
    // all we have to do is add up all the values of "is_correct" to get the total 
    $vNumberOfCorrectAnswers += $vTheAnswers[$vCounter]['is_correct'];
    // In the event the question has multiple correct answers, AND
    // if the test taker has already tried to answer this, then 
    // we need the ability to build a small array of all the answers
    // he or she has selected in order to populate the default values
    // for the checkboxes.  Remember, this is only if the question
    // allows for multiple correct answers AND the end-user is revisiting the question.
    if ($vTheAnswers[$vCounter]['isChosen']) {
      $vDefaultAnswerNumbers[] = $vTheAnswers[$vCounter]['answer_number'];
      // This next use of $vDefaultValue is a bit tricky.  What we're 
      // doing here is capturing any previously provided answer to the question.
      // The $vDefaultValue is used only for a single-answer question.  So
      // even though we're in a loop intended to capture an array of 
      // previously-provided answers to a multiple-answer question, the fact is
      // that if this is a single-answer question, there will only be one 
      // answer anyway and we'll capture it here.  Squirrelly but it works.
      $vDefaultValue           = $vTheAnswers[$vCounter]['answer_number'];
    }
    //
    $vCounter++;
  }
  
  ExamInstance::storeMessage('$vNumberOfCorrectAnswers = ' . $vNumberOfCorrectAnswers . '.', 'exam_take_form');
   
  // Very important - run the question_text through the Drupal filter to ensure
  // that the first use of the "img" tag has proper URLS. 
  // Later, if I need to capture all such references, I can create a while loop here.  
  // But I'm not currently envisioning such a requirement.
  ExamInstance::storeMessage('$vTheQuestion[\'question_text\']=' . $vTheQuestion['question_text'], 'exam_take_form');
  ExamInstance::storeMessage('Just before call to filter', 'exam_take_form');
  $vTheQuestion['question_text'] = filter_for_URL($vTheQuestion['question_text']);
  ExamInstance::storeMessage('Just after call to filter', 'exam_take_form');    
  // End of the filtering of the question text
    
  // The set of answers is treated as a single form object. 
  // It includes the question - see the "title" property.
  
  ExamInstance::storeMessage('$vDefaultValue = ' . $vDefaultValue . '.', 'exam_take_form');
  
  if ($vNumberOfCorrectAnswers == 1) {  
    // Display radio buttons: there is only one correct answer.    
    $form['pAnswer'] = array(
      '#type'          => 'radios',
      '#options'       => $vAnswerArray,
      '#title'         => t($vTheQuestion['question_text']),
      '#default_value' => $vDefaultValue 
    );
  } else {
    // Display checkboxes: there are two or more correct answers.
    $form['pAnswer'] = array(
      '#type'          => 'checkboxes',
      '#options'       => $vAnswerArray,
      '#title'         => t($vTheQuestion['question_text']),
      '#default_value' => $vDefaultAnswerNumbers
    );       
  }
   
  $form['actions'] = array('#type' => 'actions');
  if ($vCurrentQuestionNumber <> 1) {
    $form['actions']['pButtonPrevious'] = array(
      '#type'   => 'submit',
      '#value'  => t('Previous Question'),
      '#name'   => 'Previous',
      '#prefix' => '<div id="buttonGroup" class="demo">',
      '#suffix' => '</div',
      '#submit' => array('exam_capture_data'
                       , 'exam_take_form_submit_previous'
                       , 'exam_take_form_submit'
                   )
    );
  }

  if ($vCurrentQuestionNumber <> $vExamInfo['totalQuestions']) {
    $form['actions']['pButtonNext'] = array(
      '#type'   => 'submit',
      '#value'  => t('Next Question'),
      '#name'   => 'Next',
      '#prefix' => '<div id="buttonGroup" class="demo">',
      '#suffix' => '</div',
      '#submit' => array('exam_capture_data'
                       , 'exam_take_form_submit_next'
                       , 'exam_take_form_submit'
                   )
    );
  }

  $form['actions']['pButtonSummary'] = array(
    '#type'   => 'submit',
    '#value'  => t('Show Summary'),
    '#name'   => 'Summary',
    '#prefix' => '<div id="buttonGroup" class="demo">',
    '#suffix' => '</div',    
    '#submit' => array('exam_take_form_submit_summary')
  );
    
  //Do we need these?  Haven't tried them yet
  //$form['#validate'][] = 'exam_take_form_validate';
  //field_attach_form('exam', $exam, $form, $form_state);

  // Specify the function to handle form submission
  //$form['#submit'][] = 'exam_take_form_submit';
   
  return $form;
  
} // function exam_take_form()

function exam_take_oops($form) {
  
  $form['pMessage'] = array(
      '#markup' => t('<p>This exam has already been graded.</p>' .
          '<p>To take the exam again, please <a href="?q=exam/list">browse the exam list.</a></p>')
      ); 
  return $form;
} // function exam_take_oops()

/*
 * The purpose of the next three functions 
 * 
 *   - exam_take_page_advance_init()
 *   - exam_take_page_advance_set()
 *   - exam_take_page_advance_confirm()
 * 
 * is unique.  They are designed
 * to help ensure the test taker is not using the browser
 * to back up through to a previously taken exam, and 
 * ensure the test taker
 * is advancing through the test by clicking on the buttons.
 * Any backward movement should be intercepted and
 * re-routed to the Summary page.
 */

function exam_take_page_advance_init() {
  // pButtonClicked keeps track of the number of pages 
  // the test taker has clicked on. 
  // It starts with a value of "1" and is incremented 
  // by 1 with each button click.
  $_SESSION['exam']['pButtonClicked'] = 1;
  // pNextButtonClick predicts what the next page's
  // pButtonClicked value should be. 
  $_SESSION['exam']['pNextButtonClick'] = 2;
  
  return null;
  
} // function exam_take_page_advance_init()

function exam_take_page_advance_set() {
  $_SESSION['exam']['pButtonClicked']++;
} // function exam_take_page_advance_set()

/* 
 * NOTE:  See the comments for
 * the function exam_take_page_advance_init()
 *  
 */
function exam_take_page_advance_confirm($form, &$form_state) {
  
  print '<p>pButtonClicked = ' . $_SESSION['exam']['pButtonClicked'] . '</p>';
  print '<p>pNextButtonClick = ' . $_SESSION['exam']['pNextButtonClick'] . '</p>';
  
  if ($_SESSION['exam']['pButtonClicked'] == $_SESSION['exam']['pNextButtonClick']) {
    print '<p>IF</p>';
    // All is good
    $_SESSION['exam']['pNextButtonClick']++;
  } else {
    print '<p>ELSE</p>';
    // The test taker is not using buttons to advance
    // Presumably that means he or she is using the browser
    // to back up through the exam.
    // Interrupt this navigation and redirect the user to the 
    // summary page
    $form_state['redirect'] = array('exam/summary/' . $_SESSION['exam']['pExamTitleURL']);
  }
  
  return null;
  
} // function exam_take_page_advance_confirm()

function exam_take_form_submit_previous($form, &$form_state) {
  $_SESSION['exam']['pNextQuestionNumber'] = $_SESSION['exam']['pCurrentQuestionNumber'] - 1;
} // function exam_take_form_submit_previous()

function exam_take_form_submit_next($form, &$form_state) {
  $_SESSION['exam']['pNextQuestionNumber'] = $_SESSION['exam']['pCurrentQuestionNumber'] + 1;
} // function exam_take_form_submit_next()

/* 
 *  This function processes the form built in function exam_take_form()
 * 
 */
function exam_take_form_submit($form, &$form_state) {
  
  $pExamTitleURL = $_SESSION['exam']['pExamTitleURL'];
  $form_state['redirect'] = array(
    'exam/take/' . $pExamTitleURL . 
    '/' . $_SESSION['exam']['pNextQuestionNumber']
    );
  
  
  return null;
      
} // function exam_take_form_submit()

function exam_take_form_submit_summary($form, &$form_state) {

  ExamInstance::storeMessage('Entering', 'exam_take_form_submit_summary');

  exam_capture_data($form, $form_state);
  
  ExamInstance::storeMessage('Data Captured', 'exam_take_form_submit_summary');    
  
  $vExamTitleURL = $_SESSION['exam']['pExamTitleURL']; 
  $form_state['redirect'] = array(
    'exam/take/' . $vExamTitleURL . 
    '/summary');
  
  return null;
      
} // function exam_take_form_submit_summary()

function exam_capture_data($form, &$form_state) {
  
  ExamInstance::storeMessage('Entering', 'exam_capture_data');
  // Capture / record entered values
  //ExamInstance::setOneQuestionsAnswer($form, $form_state);
  ExamInstance::setOneQuestionsAnswer($form, $form_state);

} // function exam_capture_data()

function exam_take_show_summary_form($form, &$form_state) {

  ExamInstance::storeMessage('First line in this function.', 'exam_take_show_summary_form');
   
  if (isset($_SESSION['exam']['pExamInstanceID'])) {
    ExamInstance::storeMessage('The pExamInstanceID session variable is SET.', 'exam_take_show_summary_form');
    null;
  } else {
    ExamInstance::storeMessage('The pExamInstanceID session variable is NOT SET.', 'exam_take_show_summary_form');
    $form = exam_take_oops($form, $form_state);
    return $form;
  }

  $vExamInstanceID = $_SESSION['exam']['pExamInstanceID'];  
  ExamInstance::storeMessage('$vExamInstanceID = ' . $vExamInstanceID, 'exam_take_show_summary_form');
  $vExamTitleURL   = $_SESSION['exam']['pExamTitleURL'];
  ExamInstance::storeMessage('$vExamTitleURL = ' . $vExamTitleURL, 'exam_take_show_summary_form');
 
  // Get basic exam data
  $vExamInfo = ExamInstance::getInfoForExamInstanceID($vExamInstanceID);  
  $vTotalQuestions = $vExamInfo['totalQuestions'];
  $form['instructions'] = array(
      '#markup' => t('<p><b>Click any question to review it. Click "Finish and Grade" when you are finished.</b></p>')
      );
  
  $result = db_query(
     'SELECT   qi.question_instance_id
             , qi.question_no
             , qi.selected_answer_letters
             , qi.marked
      FROM     question_instance AS qi
      WHERE    qi.exam_instance_id = :eiid
      ORDER BY qi.question_no'
      , array(':eiid' => $vExamInstanceID)
    );
  
  $vContent = '';
  $vContent .= '<p><table>';
  //$vCount = 1;
  //while ($vCount <= $vTotalQuestions) {
    //ExamInstance::storeMessage('In the WHILE loop.', 'exam_take_show_summary_form');
  foreach ($result as $row){        
    
    if ($row->marked == 1) {
      $vMark = 'Mark';
    } else {
      $vMark = null;
    }
    //ExamInstance::storeMessage('The answer letter is/are: ' . $vExamInstance->ei_qa[$vCount]['selected_answer_letters'], 'exam_take_show_summary_form');
    if ($row->selected_answer_letters == '') {
        $vNoAnswer = '-no answer-';
    } else {
        $vNoAnswer = '';
    }
    $vContent .= 
        '<tr>' . 
        '<td width=120>' . 
          l( t($row->question_no . '. ' . $row->selected_answer_letters . $vNoAnswer), 
             'exam/take/' . $vExamTitleURL . '/' . $row->question_no) . 
          '</td>' .
        '<td width=80>' . t($vMark) . '</td>' . 
        '</tr>';
//    $vCount++;
  }

  $vContent .= '</table></p>';
  
  //$form['question_' . $vCount] = array(
  $form['questions'] = array(
    '#markup' => $vContent
  );
    
  $form['pButtonGradeMe'] = array(
    '#type'   => 'submit',
    '#value'  => t('Finish and Grade'),
    '#name'   => 'summary',
    '#submit' => array('exam_take_grade_submit')
  );
  
  return $form;
  
} // function exam_take_show_summary_form()


/*
 * Temporary
 */
function exam_take_grade_submit($form, &$form_state) {
  
  ExamInstance::storeMessage('Entered', 'exam_take_grade_submit');
  
  // Get the exam object  
  $vExamInstanceID = $_SESSION['exam']['pExamInstanceID'];  
  ExamInstance::storeMessage('$vExamInstanceID = ' . $vExamInstanceID . '.', 'exam_take_grade_submit');
  $vExamTitleURL   = $_SESSION['exam']['pExamTitleURL'];
  ExamInstance::storeMessage('$vExamTitleURL = ' . $vExamTitleURL . '.', 'exam_take_grade_submit');
  
  // First, grade each individual answer
  // Here's the thinking on this update statement. 
  // If an answer should be marked as correct,
  // is_correct should be 1.  Otherwise 0.
  // If an answer was answered correctly,
  // isChosen should be 1.  Otherwise 0.
  // So these are the only options for any one answer:
  //
  //  is_correct  isChosen       ADD  Meaning
  //  ----------  --------  --------  -------
  //           0         0         0  Correct response
  //           0         1         1  Wrong response
  //           1         0         1  Wrong response
  //           1         1         2  Correct response
  //
  // Therefore, if we just sum the two numbers for any one answer,
  // we can use a CASE statement for each answer to translate the
  // meaning into a numeric flag indicating if the response
  // to that one answer is correct (1) or incorrect (0).  
  $vUpdateStatement =
    'UPDATE answer_instance AS ai
        SET respondedCorrectly = 
              (CASE (is_correct + isChosen) 
                 WHEN 0 THEN 1 
                 WHEN 1 THEN 0
                 WHEN 2 THEN 1
                 ELSE      NULL
                 END)
     WHERE question_instance_id IN 
             (SELECT qi.question_instance_id
              FROM   question_instance qi
              WHERE  qi.exam_instance_id = :eiid)';
  $num_updated = db_query($vUpdateStatement, array(':eiid' => $vExamInstanceID));
  
  // Next, roll up the grade to the question level
  $vUpdateStatement = 
    'UPDATE question_instance AS A
        SET A.answeredCorrectly = 
              (SELECT   (CASE(SUM(B.respondedCorrectly))
                           WHEN 4 THEN 1
                           ELSE        0
                           END) 
               FROM     answer_instance B
               WHERE    B.question_instance_id = A.question_instance_id
                 AND    B.respondedCorrectly IS NOT NULL
               GROUP BY B.question_instance_id)
     WHERE A.exam_instance_id = :eiid';
  $num_updated = db_query($vUpdateStatement, array(':eiid' => $vExamInstanceID));

  // Next, grade the overall exam.
  $vUpdateStatement = 
    'UPDATE exam_instance 
        SET numberCorrect = 
              (SELECT COUNT(CASE WHEN B.answeredCorrectly = 1 THEN 1 END)
               FROM   question_instance AS B
               WHERE  B.exam_instance_id = :eiid)
          , numberWrong   = 
              (SELECT COUNT(CASE WHEN C.answeredCorrectly = 0 THEN 1 END)
               FROM   question_instance AS C
               WHERE  C.exam_instance_id = :eiid)
          , numberUnanswered = 
              (SELECT COUNT(CASE WHEN D.answeredCorrectly IS NULL THEN 1 END)
               FROM   question_instance AS D
               WHERE  D.exam_instance_id = :eiid)
          , grade = 
              (SELECT ROUND((COUNT(CASE WHEN B.answeredCorrectly = 1 THEN 1 END)
                             / 
                             COUNT(*)*100),2) 
                            AS grade
               FROM   question_instance AS B
               WHERE  B.exam_instance_id = :eiid)
          , graded = 1
     WHERE  exam_instance_id = :eiid';
  $num_updated = db_query($vUpdateStatement, array(':eiid' => $vExamInstanceID));
  
  $form_state['redirect'] = array('exam/grade/' . $vExamTitleURL);  
  
} // function exam_take_grade_submit()


function exam_take_show_grade_form($form, &$form_state) {
  
  if (isset($_SESSION['exam']['pExamInstanceID'])) {
    $pExamInstanceID = $_SESSION['exam']['pExamInstanceID'];    
  } else {
    $form['somethingIsWrong'] = array(
    '#markup' => t('I\'m sorry but I\'m not sure what you\'re trying to do here.  Please visit the menu and make another selection.'),
    '#weight' => '20',
     );
    return $form; 
  }

  $vContent = '';
  
  $vExamInfo = ExamInstance::getInfoForExamInstanceID($pExamInstanceID);
  // SKERE777 - ERROR CHECK ON INCOMING DATA, POSSIBLE INVALID EXAM INSTANCE ID
  // exam_take_oops($form, $form_state);
  
  // Extract the data needed for this function
  $vTotalQuestions = $vExamInfo['totalQuestions']; 
  $vNumberCorrect  = $vExamInfo['numberCorrect'];
  $vGrade          = $vExamInfo['grade']; 

  // Show the grade.  Use the HTML code for a percent sign after the grade.
  $form['pTheGrade'] = array(
    '#markup' => '<p><h1>Your Grade: ' . $vGrade . '&#37;</h1><p>'
  ); 
  
  if ($vNumberCorrect == 1) {
    $vQuestionText = 'question';
  } else {
    $vQuestionText = 'questions';
  }
  
  $vContent .= '<p>' . l(t('Click to see a personalized report.'), 'exam/report/' . $pExamInstanceID) . '</p>';
        
  $vContent .= '<p>You answered ' . 
      $vNumberCorrect . 
      ' ' . $vQuestionText . ' correctly out of a total of ' .
      $vTotalQuestions . ' questions.</p>';
   
  $form['pTheResult'] = array(
    '#markup' => $vContent,
    '#weight' => '5',
  );  
  
  //$form['pDone'] = array(
  //  '#access' => user_access('view exam'),
  //  '#type'   => 'submit',
  //  '#value'  => t('Browse the exam list'),
  //  '#name'   => 'Summary',
  //  '#submit' => array('exam_concluded')
  //);   

  // Now that the grade is displayed, let's get some details
  // and add that to the display.  Let's list a summary
  // of the questions that were answered incorrectly
  // by their topic area, in order of the topic on which
  // the test-taker needs to do the most work

  // Zero out $vCount and let's re-use it
  
  $vContent = ExamInstance::getExamResultsByTopic($pExamInstanceID);

  $form['theTopicList'] = array(
    '#markup' => t($vContent),
    '#weight' => '10',
  );
  
  // Display a final set of suggestions
  $vContent = '';
  $vContent .= 
        t('<hr>You can ') 
      . l(t('review your exam history'), 'exam/history') 
      . t(', or ') 
      . l(t('take another exam.'), 'exam/list') 
      . t('.');  
  
  $form['theSuggestions'] = array(
    '#markup' => t($vContent),
    '#weight' => '20',
  );  
  
  // Unset session variables from the exam session
  ExamInstance::examCleanup();
  
  return $form;
  
} // function exam_take_show_grade_form()

function exam_take_grade_report_form($form, &$form_state) {
  
  if (isset($_SESSION['exam']['pExamInstanceID'])) {
    $pExamInstanceID = $_SESSION['exam']['pExamInstanceID'];    
  } else {
    $form['somethingIsWrong'] = array(
    '#markup' => t('I\'m sorry but I\'m not sure what you\'re trying to do here.  Please visit the menu and make another selection.'),
    '#weight' => '20',
     );
    return $form; 
  }

  $vExamInfo = ExamInstance::getExamResultsPersonalizedReport($pExamInstanceID);
  // SKERE777 - ERROR CHECK ON INCOMING DATA, POSSIBLE INVALID EXAM INSTANCE ID
  // exam_take_oops($form, $form_state);
  
  // Extract the data needed for this function
  $vTotalQuestions = $vExamInfo['totalQuestions']; 
  $vNumberCorrect  = $vExamInfo['numberCorrect'];
  $vGrade          = $vExamInfo['grade']; 

  // Show the grade.  Use the HTML code for a percent sign after the grade.
  $form['pTheGrade'] = array(
    '#markup' => '<p><h1>Your Grade: ' . $vGrade . '&#37;</h1><p>'
  ); 
  
  if ($vNumberCorrect == 1) {
    $vQuestionText = 'question';
  } else {
    $vQuestionText = 'questions';
  }
  
  $vContent = '<p>You answered ' . 
      $vNumberCorrect . 
      ' ' . $vQuestionText . ' correctly out of a total of ' .
      $vTotalQuestions . ' questions.</p>';
   
  $form['pTheResult'] = array(
    '#markup' => $vContent,
    '#weight' => '5',
  );  
  
  //$form['pDone'] = array(
  //  '#access' => user_access('view exam'),
  //  '#type'   => 'submit',
  //  '#value'  => t('Browse the exam list'),
  //  '#name'   => 'Summary',
  //  '#submit' => array('exam_concluded')
  //);   

  // Now that the grade is displayed, let's get some details
  // and add that to the display.  Let's list a summary
  // of the questions that were answered incorrectly
  // by their topic area, in order of the topic on which
  // the test-taker needs to do the most work

  // Zero out $vCount and let's re-use it
  
  $vContent = ExamInstance::getExamResultsByTopic($pExamInstanceID);

  $form['theReport'] = array(
    '#markup' => t($vContent),
    '#weight' => '10',
  );
  
  // Display a final set of suggestions
  $vContent = '';
  $vContent .= 
        t('<hr>You can ') 
      . l(t('review your exam history'), 'exam/history') 
      . t(', or ') 
      . l(t('take another exam.'), 'exam/list') 
      . t('.');  
  
  $form['theSuggestions'] = array(
    '#markup' => t($vContent),
    '#weight' => '20',
  );  
  
  return $form;
  
} // function exam_take_grade_report_form()

/**
 * function exam_list
 * 
 * Stored in: exam.pages.inc
 * 
 * Note:  This function assumes that at least one exam has already been
 * entered into the database. 
 *
 * 
 * @param $view_mode
 *   The view mode we want to display.
 */
function exam_list() {
  
  //$vNumberOfColumns = 4;
  
  $vContent = t(ExamInstance::getListingOfExams());
  
  return $vContent;
  
  //foreach ($links as $link) {
  //  $items[] = l($link['title'], $link['href'], $item['localized_options'])
  //    . ': ' . filter_xss_admin($link['description']);
  //}

  //return theme('item_list', array('items' => $items));
 
} // function exam_list()

function exam_record_display($pExamInstanceID) {
  
  $vContent = ExamInstance::getExamResultsByTopic($pExamInstanceID);
      
  return $vContent;
  
} // function exam_record_display()

function exam_report_display($pExamInstanceID) {
  
  $vContent = ExamInstance::getExamResultsPersonalizedReport($pExamInstanceID);
      
  return $vContent;
  
} // function exam_report_display()

/* 
 * This function rebuilds the menu.
 * This is useful when first adding a new exam to the system
 * manually - that is, by running SQL scripts to populate
 * the database.  After running a script that adds a new 
 * exam to the database, use this menu option to manually 
 * rebuild the menus.  This will add the exams as menu options.
 * 
 */
function exam_menu_rebuild() {
   
  menu_rebuild();
      
  return 'Menu rebuild completed.';
} // function exam_menu_rebuild()

function exam_test_one($form, &$form_state) {
  $vTest = 'Hi from test one';
  $vTestObject = new ExamTest();
  $vTestObject->setTestVariable(11);
  $_SESSION['exam']['pTestObjectByValue'] = $vTestObject;
  $_SESSION['exam']['pTestObjectByReference'] =& $vTestObject;
  $vTestObject->setTestVariable(2222);
  $vTest .= $vTestObject->getTestVariable() . '!';
  $vTestObjectSerialized = serialize($vTestObject);
  //$form = array();
  $form['pMessage'] = array(
    '#markup' => 'This is it - ' . $vTest,
    '#weight' => '5',
  );  
  $form['pPassSimple'] = array(
    '#type' => 'hidden',
    '#value' => 'THIS IS SIMPLE'
  );  
  $form['pPassIt'] = array(
    '#type' => 'hidden',
    '#value' => $vTestObjectSerialized
  );
  $form['buttons']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
    '#weight' => 5,
    '#submit' => array('exam_test_two'), 
  );
  return $form;
}

function exam_test_two($form, &$form_state) {
  $form_state['redirect'] = 'exam/test/three';
}

function exam_test_three($form, &$form_state) {
  
  $vFirstObject = $_SESSION['exam']['pTestObjectByValue'];
  $vFirstValue = $vFirstObject->getTestVariable();
  $vSecondObject = $_SESSION['exam']['pTestObjectByValue'];
  $vSecondValue = $vSecondObject->getTestVariable();
  $vContent = 'First is ' . $vFirstValue . '.';
  $vContent .= 'Second is ' . $vSecondValue . ' and that is all.';
      
  //$vPassSimple = $form_state['values']['pPassSimple'];  
  //$vPassIt     = $form_state['values']['pPassIt'];  
  //$vGotIt      = unserialize($vPassIt);
  $form['pMessage'] = array(
    '#markup' => 'Part 2.  vPassSimple is ' . $vContent
  );  
  return $form;
}

function filter_for_URL($pText) {
  
  ExamInstance::storeMessage('$pText = ' . $pText, 'filter_for_URL');  
  $vMatch = array();
  // $vRegEx_ImgURL extracts the URL from an image tag
  $vRegEx_ImgURL  = '&(?<=<img src=)(.*?)(?=>|\s)&';
  // $vRegEx_ImgHTML extracts an entire HTML image 
  // tag (img) from start to finish, including the angle brackets
  $vRegEx_ImgHTML = '&(<img src=)(.*?)(>)&';
  
  ExamInstance::storeMessage('Before while loop.', 'filter_for_URL');
  
  while (preg_match($vRegEx_ImgURL, $pText, $vMatch) == 1) {
    ExamInstance::storeMessage('In the while loop.', 'filter_for_URL');
    $vImageURL = $vMatch[0];
    ExamInstance::storeMessage('$vImageURL = ' . $vImageURL . '!', 'filter_for_URL');
    $vImageExhibit = array(
        'path'       => $vImageURL
      , 'alt'        => 'Image for exam'
      , 'title'      => 'Image for exam'      
      , 'attributes' => array('border' => '0')
    );    

    ExamInstance::storeMessage('Just finished created vImageExhibit', 'filter_for_URL');
  
    $vImageURLThemed = theme('image', $vImageExhibit);
    ExamInstance::storeMessage('$vImageURLThemed = ' . $vImageURLThemed . '!', 'filter_for_URL');
    $pText = preg_replace($vRegEx_ImgHTML, $vImageURLThemed, $pText, 1);
  }

  ExamInstance::storeMessage('After while loop', 'filter_for_URL');
  
  return $pText;
}

/**
 * Initialization of jquery_loadinganimation (on each page).
 * 
 * Code snippets extracted from the 
 * Drupal module "jquery_loadinganimation"
 * 
 */
function jquery_loadinganimation_init() {
  $module_path = drupal_get_path('module', 'exam');
  $js_settings = _jquery_loadinganimation_get_settings();
  drupal_add_js(array('jquery_loadinganimation' => $js_settings), 'setting');
  drupal_add_js($module_path . '/js/jquery_loadinganimation.js');
  drupal_add_css($module_path . '/css/jquery_loadinganimation.css');
}

/**
 * Specifies the commerce_powl AJAX Settings.
 *
 * @return array
 *   Array of configuration variables to be made available in JavaScript.
 * 
 * Code snippets extracted from the 
 * Drupal module "jquery_loadinganimation"
 * 
 */
function _jquery_loadinganimation_get_settings() {
  $module_path = drupal_get_path('module', 'exam');
  return array(
    'module_path' => $module_path,
    'img_src' => variable_get('jquery_loadinganimation_img_src', $module_path . '/images/jquery_loadinganimation/ajax-loader.gif'),
    'show_on_ajax' => variable_get('jquery_loadinganimation_show_on_ajax', FALSE),
    'show_on_href' => variable_get('jquery_loadinganimation_show_on_href', TRUE),
	'show_on_form_submit' => variable_get('jquery_loadinganimation_show_on_form_submit', TRUE),
    'close_on_click' => variable_get('jquery_loadinganimation_close_on_click', TRUE),
    'close_on_esc' => variable_get('jquery_loadinganimation_close_on_esc', TRUE),
    'subselector' => variable_get('jquery_loadinganimation_subselector', ''));
}

/**
 * Implements hook_install().
 * 
 * Code snippets extracted from the 
 * Drupal module "jquery_loadinganimation"
 * 
 */
function jquery_loadinganimation_install() {
  // Set general settings.
  variable_set('jquery_loadinganimation_show_on_ajax', FALSE);
  variable_set('jquery_loadinganimation_show_on_href', TRUE);
  variable_set('jquery_loadinganimation_show_on_form_submit', TRUE);
  variable_set('jquery_loadinganimation_close_on_click', TRUE);
  variable_set('jquery_loadinganimation_close_on_esc', TRUE);
  variable_set('jquery_loadinganimation_subselector', '');
}
