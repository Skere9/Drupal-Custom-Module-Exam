<?php

/* 
 * The exam.pages.basic.inc file
 * 
 * Contains files that are either based on the examples in a book
 * I have been reading, or were among the first pages I created
 * soon after reading the book, and the examples as a basis in 
 * creating additional functionality.
 * 
 * The book:
 * Drupal 7 Module Development
 * by Packt Publishing
 * Authors Matt Butcher and Larry Garfield, John Wilkins, Matt Farina, Ken Rickard, Greg Dunlap
 * Published December 3, 2010
 * 420 pages
 * 
 */

/**
 * Menu callback; Show list of exam types we can add.
 */
function exam_add_page() {
  $item = menu_get_item();
  $links = system_admin_menu_block($item);

  foreach ($links as $link) {
    $items[] = l($link['title'], $link['href'], $item['localized_options'])
      . ': ' . filter_xss_admin($link['description']);
  }

  return theme('item_list', array('items' => $items));
} // function exam_add_page()

/**
 * Present an exam submission form.
 */
function exam_add($type) {
  global $user;

  $types = exam_types();
  $type = isset($type) ? str_replace('-', '_', $type) : NULL;
  if (empty($types[$type])) {
    return MENU_NOT_FOUND;
  }

  $exam = entity_get_controller('exam')->create($type);

  drupal_set_title(t('Create @name', array('@name' => $types[$type]->name)), PASS_THROUGH);
  return drupal_get_form($type . '_exam_form', $exam);
} // function exam_add()

/**
 * Present a topic submission form.
 */
function topic_add() {
  global $user; // SKERE - Is this necessary?  Reconcile with declaration in function exam_add()

  $topic = entity_get_controller('topic'); // Watch this line

  drupal_set_title(t('Create topic'), PASS_THROUGH);
  return drupal_get_form('topic_form', $topic);
} // function topic_add()

/**
 * Menu callback; presents the exam editing form, or redirects to delete confirmation.
 *
 * @param $exam
 *   The exam object to edit.
 */
function exam_page_edit($exam) {
  $types = exam_types();
  drupal_set_title(t('<em>Edit @type</em> @title', array('@type' => $types[$exam->type]->name, '@title' => $exam->title)), PASS_THROUGH);

  return drupal_get_form($exam->type . '_exam_form', $exam);
} // function exam_page_edit()

/**
 * Menu callback; presents the topic editing form, or redirects to delete confirmation.
 *
 * @param $topic
 *   The topic object to edit.
 */
function topic_page_edit($topic) {
  drupal_set_title(t('<em>Edit </em> @title', $topic->title), PASS_THROUGH);

  return drupal_get_form('topic_form', $topic);
} // function topic_page_edit()

/**
 * Form builder; Displays the exam add/edit form.
 *
 * @param $form
 * @param $form_state
 * @param $exam
 *   The exam object to edit, which may be brand new.
 */
function exam_form($form, &$form_state, $exam) {

  // Set the id and identify this as an exam edit form.
  $form['#id'] = 'exam-form';

  // Save the exam for later, in case we need it.
  $form['#exam'] = $exam;
  $form_state['exam'] = $exam;

  // Common fields. We don't have many.
  $form['exam_title'] = array(
    '#type' => 'textfield',
    '#title' => t('Exam Title'),
    '#default_value' => $exam->exam_title,
    '#weight' => -8,
    '#required' => TRUE,
  );

  $form['exam_description'] = array(
    '#type' => 'textfield',
    '#title' => t('Exam Description'),
    '#default_value' => $exam->exam_description,
    '#weight' => -3,
    '#required' => FALSE,
  );

  $form['status'] = array(
    '#type' => 'textfield',
    '#title' => t('Status'),
    '#default_value' => $exam->status,
    '#weight' => 20,
    '#maxlength' => 4,
    '#size' => 4,
    '#required' => TRUE,
  );

  $form['revision'] = array(
    '#access' => user_access('administer exams'),
    '#type' => 'checkbox',
    '#title' => t('Create new revision'),
    '#default_value' => 0,
  );

  // Add the buttons.
  $form['buttons'] = array();
  $form['buttons']['#weight'] = 100;
  $form['buttons']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
    '#weight' => 5,
    '#submit' => array('exam_form_submit'),
  );
  if (!empty($exam->exam_id)) {
    $form['buttons']['delete'] = array(
      '#access' => user_access('delete exams'),
      '#type' => 'submit',
      '#value' => t('Delete'),
      '#weight' => 15,
      '#submit' => array('exam_form_delete_submit'),
    );
  }

  $form['#validate'][] = 'exam_form_validate';

  field_attach_form('exam', $exam, $form, $form_state);

  return $form;
} // function exam_form()

function exam_form_validate($form, &$form_state) {
  $exam = $form_state['exam'];

  // Field validation.
  field_attach_form_validate('exam', $exam, $form, $form_state);
} // function exam_form_validate()

function exam_form_submit($form, &$form_state) {
  global $user;

  $exam = &$form_state['exam'];

  // Set the exam's uid if it's being created at this time.
  if (empty($exam->uid)) {
    $exam->uid = $user->uid;
  }

  $exam->exam_title = $form_state['values']['exam_title'];
  $exam->exam_description = $form_state['values']['exam_description'];
  $exam->revision = $form_state['values']['revision'];
  $exam->status = $form_state['values']['status'];

  // Notify field widgets.
  field_attach_submit('exam', $exam, $form, $form_state);

  // Save the exam.
  exam_save($exam);

  // Notify the user.
  drupal_set_message(t('Exam saved.'));

  $form_state['redirect'] = 'exam/' . $exam->exam_id;
} // function exam_form_submit()


function exam_form_delete_submit($form, &$form_state) {
  $destination = array();
  if (isset($_GET['destination'])) {
    $destination = drupal_get_destination();
    unset($_GET['destination']);
  }
  $exam = $form['#exam'];
  $form_state['redirect'] = array('exam/' . $exam->exam_id . '/delete', array('query' => $destination));
} // function exam_form_delete_submit()

/**
 * Form builder; Displays the topic add/edit form.
 *
 * @param $form
 * @param $form_state
 * @param $topic
 *   The topic object to edit, which may be brand new.
 */
function topic_form($form, &$form_state, $topic) {

  // Set the id and identify this as a topic edit form.
  $form['#id'] = 'topic-form';

  // Save the topic for later, in case we need it.
  $form['#topic'] = $topic;
  $form_state['topic'] = $topic;

  // Common fields. We don't have many.
  $form['exam_id'] = array(
    '#type' => 'textfield',
    '#title' => t('Exam ID'),
    '#default_value' => '',
    '#weight' => -9,
    '#maxlength' => 4,
    '#size' => 4,
    '#required' => TRUE,
  );

  $form['topic_title'] = array(
    '#type' => 'textfield',
    '#title' => t('Topic Title'),
    '#default_value' => '', // $topic->topic_title,
    '#weight' => 3,
    '#required' => TRUE,
  );

  $form['status'] = array(
    '#type' => 'textfield',
    '#title' => t('Status'),
    '#default_value' => '', //$topic->status,
    '#weight' => 20,
    '#maxlength' => 4,
    '#size' => 4,
    '#required' => TRUE,
  );

  $form['revision'] = array(
    '#access' => user_access('administer topics'),
    '#type' => 'checkbox',
    '#title' => t('Create new revision'),
    '#default_value' => 0,
  );

  // Add the buttons.
  $form['buttons'] = array();
  $form['buttons']['#weight'] = 100;
  $form['buttons']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
    '#weight' => 5,
    '#submit' => array('topic_form_submit'),
  );
  if (!empty($topic->topic_id)) {
    $form['buttons']['delete'] = array(
      '#access' => user_access('delete topics'),
      '#type' => 'submit',
      '#value' => t('Delete'),
      '#weight' => 15,
      '#submit' => array('topic_form_delete_submit'),
    );
  }

  $form['#validate'][] = 'topic_form_validate';

  field_attach_form('topic', $topic, $form, $form_state);

  return $form;
} // function topic_form()

function topic_form_validate($form, &$form_state) {
  $topic = $form_state['topic'];

  // Field validation.
  field_attach_form_validate('topic', $topic, $form, $form_state);
} // function topic_form_validate()

function topic_form_submit($form, &$form_state) {
  global $user;

  $topic = &$form_state['topic'];

  // Set the topic's uid if it's being created at this time.
  if (empty($topic->uid)) {
    $topic->uid = $user->uid;
  }

  $topic->exam_id = $form_state['values']['exam_id'];
  $topic->topic_title = $form_state['values']['topic_title'];
  $topic->revision = $form_state['values']['revision'];
  $topic->status = $form_state['values']['status'];

  // Notify field widgets.
  field_attach_submit('topic', $topic, $form, $form_state);

  // Save the topic.
  topic_save($topic);

  // Notify the user.
  drupal_set_message(t('Topic saved.'));

  $form_state['redirect'] = 'topic/' . $topic->topic_id;
} // function topic_form_submit()


function topic_form_delete_submit($form, &$form_state) {
  $destination = array();
  if (isset($_GET['destination'])) {
    $destination = drupal_get_destination();
    unset($_GET['destination']);
  }
  $topic = $form['#topic'];
  $form_state['redirect'] = array('topic/' . $topic->topic_id . '/delete', array('query' => $destination));
} // function topic_form_delete_submit()

/**
 * Displays an exam.
 *
 * @param $exam
 *   The exam object to display.
 * @param $view_mode
 *   The view mode we want to display.
 */
function exam_page_view($exam, $view_mode = 'full') {
  // Remove previously built content, if exists.
  $exam->content = array();

  if ($view_mode == 'teaser') {
    $exam->content['title'] = array(
      '#markup' => filter_xss($exam->title),
      '#weight' => -5,
    );
  }

  // Build fields content.
  field_attach_prepare_view('exam', array($exam->exam_id => $exam), $view_mode);
  entity_prepare_view('exam', array($exam->exam_id => $exam));
  $exam->content += field_attach_view('exam', $exam, $view_mode);
  
  return $exam->content;
} // function exam_page_view()

/**
 * Form bulder; Asks for confirmation of exam deletion.
 */
function exam_delete_confirm($form, &$form_state, $exam) {
  $form['#exam'] = $exam;
  // Always provide entity id in the same form key as in the entity edit form.
  $form['exam_id'] = array('#type' => 'value', '#value' => $exam->exam_id);
  return confirm_form($form,
    t('Are you sure you want to delete %title?', array('%title' => $exam->title)),
    'exam/' . $exam->exam_id,
    t('This action cannot be undone.'),
    t('Delete'),
    t('Cancel')
  );
} // function exam_delete_confirm()

/**
 * Executes exam deletion.
 */
function exam_delete_confirm_submit($form, &$form_state) {
  if ($form_state['values']['confirm']) {
    $exam = exam_load($form_state['values']['exam_id']);
    exam_delete($form_state['values']['exam_id']);
    watchdog('exam', '@type: deleted %title.', array('@type' => $exam->type, '%title' => $exam->title));

    $types = exam_types();
    drupal_set_message(t('@type %title has been deleted.', array('@type' => $types[$exam->type]->name, '%title' => $exam->title)));
  }

  $form_state['redirect'] = '<front>';
} // function exam_delete_confirm_submit()

/**
 * Menu callback; Displays a listing of recent exams.
 *
 * This doesn't really work yet because our presentation code doesn't show
 * the title.
 */
function exam_page_list_recent() {
  $content = array();

  $query = new EntityFieldQuery();
  $query
    ->entityCondition('entity_type', 'exam')
    ->propertyOrderBy('created', 'DESC')
    ->fieldCondition('field_artist', 'value', 'Da Vinci', 'CONTAINS', 0)
    ->range(0, 5);
  $result = $query->execute();

  $exams = exam_load_multiple(array_keys($result['exam']));
  foreach ($exams as $exam) {
    $content[$exam->exam_id] = exam_page_view($exam, 'teaser');
  }

  return $content;
} // function exam_page_list_recent()

/**
 * Displays a topic.
 *
 * @param $topic
 *   The topic object to display.
 * @param $view_mode
 *   The view mode we want to display.
 */
function topic_page_view($topic, $view_mode = 'full') {
  // Remove previously built content, if exists.
  $topic->content = array();

  if ($view_mode == 'teaser') {
    $topic->content['title'] = array(
      '#markup' => filter_xss($topic->title),
      '#weight' => -5,
    );
  }

  // Build fields content.
  field_attach_prepare_view('topic', array($topic->topic_id => $topic), $view_mode);
  entity_prepare_view('topic', array($topic->topic_id => $topic));
  $topic->content += field_attach_view('topic', $topic, $view_mode);
  
  return $topic->content;
} // function topic_page_view()

/**
 * Present a question submission form.
 */
function question_add() {
  global $user; // SKERE - Is this necessary?  Reconcile with declaration in function exam_add()

  $question = entity_get_controller('question'); // Watch this line

  drupal_set_title(t('Create question'), PASS_THROUGH);
  return drupal_get_form('question_form', $question);
} // function question_add()

/**
 * Form builder; Displays the question add/edit form.
 *
 * @param $form
 * @param $form_state
 * @param $question
 *   The question object to edit, which may be brand new.
 */
function question_form($form, &$form_state, $question) {

  // Set the id and identify this as a question edit form.
  $form['#id'] = 'question-form';

  // Save the question for later, in case we need it.
  $form['#question'] = $question;
  $form_state['question'] = $question;

  // Common fields. We don't have many.
  $form['exam_id'] = array(
    '#type' => 'textfield',
    '#title' => t('Exam ID'),
    '#default_value' => '',
    '#weight' => -20,
    '#maxlength' => 4,
    '#size' => 4,
    '#required' => TRUE,
  );

  $form['topic_id'] = array(
    '#type' => 'textfield',
    '#title' => t('Topic ID'),
    '#default_value' => '',
    '#weight' => -10,
    '#maxlength' => 4,
    '#size' => 4,
    '#required' => TRUE,
  );

  $form['question_text'] = array(
    '#type' => 'textfield',
    '#title' => t('Question Text'),
    '#default_value' => '', // $question->question_text,
    '#weight' => 3,
    '#required' => TRUE,
  );

  $form['status'] = array(
    '#type' => 'textfield',
    '#title' => t('Status'),
    '#default_value' => '', //$question->status,
    '#weight' => 20,
    '#maxlength' => 4,
    '#size' => 4,
    '#required' => TRUE,
  );

  $form['revision'] = array(
    '#access' => user_access('administer questions'),
    '#type' => 'checkbox',
    '#title' => t('Create new revision'),
    '#default_value' => 0,
  );

  // Add the buttons.
  $form['buttons'] = array();
  $form['buttons']['#weight'] = 100;
  $form['buttons']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
    '#weight' => 5,
    '#submit' => array('question_form_submit'),
  );
  if (!empty($question->question_id)) {
    $form['buttons']['delete'] = array(
      '#access' => user_access('delete questions'),
      '#type' => 'submit',
      '#value' => t('Delete'),
      '#weight' => 15,
      '#submit' => array('question_form_delete_submit'),
    );
  }

  $form['#validate'][] = 'question_form_validate';

  field_attach_form('question', $question, $form, $form_state);

  return $form;
} // function question_form()

function question_form_delete_submit($form, &$form_state) {
  $destination = array();
  if (isset($_GET['destination'])) {
    $destination = drupal_get_destination();
    unset($_GET['destination']);
  }
  $question = $form['#question'];
  $form_state['redirect'] = array('question/' . $question->question_id . '/delete', array('query' => $destination));
} // function question_form_delete_submit()

function question_form_submit($form, &$form_state) {
  global $user;

  $question = &$form_state['question'];

  // Set the question's uid if it's being created at this time.
  if (empty($question->uid)) {
    $question->uid = $user->uid;
  }

  $question->exam_id = $form_state['values']['exam_id'];
  $question->topic_id = $form_state['values']['topic_id'];
  $question->question_text = $form_state['values']['question_text'];
  $question->revision = $form_state['values']['revision'];
  $question->status = $form_state['values']['status'];

  // Notify field widgets.
  field_attach_submit('question', $question, $form, $form_state);

  // Save the question.
  question_save($question);

  // Notify the user.
  drupal_set_message(t('Question saved.'));

  $form_state['redirect'] = 'question/' . $question->question_id;
} // function question_form_submit()

function question_form_validate($form, &$form_state) {
  $question = $form_state['question'];

  // Field validation.
  field_attach_form_validate('question', $question, $form, $form_state);
} // function question_form_validate()

/**
 * Displays a question.
 *
 * @param $question
 *   The question object to display.
 * @param $view_mode
 *   The view mode we want to display.
 */
function question_page_view($question, $view_mode = 'full') {
  // Remove previously built content, if exists.
  $question->content = array();

  if ($view_mode == 'teaser') {
    $question->content['title'] = array(
      '#markup' => filter_xss($question->title),
      '#weight' => -5,
    );
  }

  // Build fields content.
  field_attach_prepare_view('question', array($question->question_id => $question), $view_mode);
  entity_prepare_view('question', array($question->question_id => $question));
  $question->content += field_attach_view('question', $question, $view_mode);
  
  return $question->content;
} // function question_page_view()

/**
 * Present an answer submission form.
 */
function answer_add() {
  global $user; // SKERE - Is this necessary?  Reconcile with declaration in function answer_add()

  $answer = entity_get_controller('answer'); // Watch this line

  drupal_set_title(t('Create answer'), PASS_THROUGH);
  return drupal_get_form('answer_form', $answer);
} // function answer_add()

/**
 * Form builder; Displays the answer add/edit form.
 *
 * @param $form
 * @param $form_state
 * @param $answer
 *   The answer object to edit, which may be brand new.
 */
function answer_form($form, &$form_state, $answer) {

  // Set the id and identify this as an answer edit form.
  $form['#id'] = 'answer-form';

  // Save the answer for later, in case we need it.
  $form['#answer'] = $answer;
  $form_state['answer'] = $answer;

  // Common fields. We don't have many.

  $form['question_id'] = array(
    '#type' => 'textfield',
    '#title' => t('Question ID'),
    '#default_value' => '',
    '#weight' => -10,
    '#maxlength' => 4,
    '#size' => 4,
    '#required' => TRUE,
  );

  $form['answer_text'] = array(
    '#type' => 'textfield',
    '#title' => t('Answer Text'),
    '#default_value' => '', // $answer->answer_text,
    '#weight' => 3,
    '#required' => TRUE,
  );

  $form['is_correct'] = array(
    '#type' => 'checkbox',
    '#title' => t('Is this a correct answer? (Checked = YES)'),
    '#default_value' => 0, //$answer->is_correct,
    //'#value' => '1', //1 = YES, 0 = NO
    '#weight' => 8,
  );

  $form['rationale'] = array(
    '#type' => 'textfield',
    '#title' => t('Rationale'),
    '#default_value' => '', // $answer->rationale,
    '#weight' => 10,
    '#required' => TRUE,
  );

  $form['status'] = array(
    '#type' => 'textfield',
    '#title' => t('Status'),
    '#default_value' => '', //$answer->status,
    '#weight' => 20,
    '#maxlength' => 4,
    '#size' => 4,
    '#required' => TRUE,
  );

  $form['revision'] = array(
    '#access' => user_access('administer answers'),
    '#type' => 'checkbox',
    '#title' => t('Create new revision'),
    '#default_value' => 0,
    '#weight' => 40,
  );

  // Add the buttons.
  $form['buttons'] = array();
  $form['buttons']['#weight'] = 100;
  $form['buttons']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
    '#weight' => 5,
    '#submit' => array('answer_form_submit'),
  );
  if (!empty($answer->answer_id)) {
    $form['buttons']['delete'] = array(
      '#access' => user_access('delete answers'),
      '#type' => 'submit',
      '#value' => t('Delete'),
      '#weight' => 15,
      '#submit' => array('answer_form_delete_submit'),
    );
  }

  $form['#validate'][] = 'answer_form_validate';

  field_attach_form('answer', $answer, $form, $form_state);

  return $form;
} // function answer_form()

function answer_form_delete_submit($form, &$form_state) {
  $destination = array();
  if (isset($_GET['destination'])) {
    $destination = drupal_get_destination();
    unset($_GET['destination']);
  }
  $answer = $form['#answer'];
  $form_state['redirect'] = array('answer/' . $answer->answer_id . '/delete', array('query' => $destination));
} // function answer_form_delete_submit()

function answer_form_submit($form, &$form_state) {
  global $user;

  $answer = &$form_state['answer'];

  // Set the answer's uid if it's being created at this time.
  if (empty($answer->uid)) {
    $answer->uid = $user->uid;
  }
  
  $answer->question_id = $form_state['values']['question_id'];
  $answer->answer_text = $form_state['values']['answer_text'];
  $answer->is_correct  = $form_state['values']['is_correct'];
  $answer->rationale   = $form_state['values']['rationale'];
  $answer->revision    = $form_state['values']['revision'];
  $answer->status      = $form_state['values']['status'];

  // Notify field widgets.
  field_attach_submit('answer', $answer, $form, $form_state);

  // Save the answer.
  answer_save($answer);

  // Notify the user.
  drupal_set_message(t('Answer saved.'));

  $form_state['redirect'] = 'answer/' . $answer->answer_id;
} // function answer_form_submit()

function answer_form_validate($form, &$form_state) {
  $answer = $form_state['answer'];

  // Field validation.
  field_attach_form_validate('answer', $answer, $form, $form_state);
} // function answer_form_validate()

/**
 * Displays an answer.
 *
 * @param $answer
 *   The answer object to display.
 * @param $view_mode
 *   The view mode we want to display.
 */
function answer_page_view($answer, $view_mode = 'full') {
  // Remove previously built content, if exists.
  $answer->content = array();

  if ($view_mode == 'teaser') {
    $answer->content['title'] = array(
      '#markup' => filter_xss($answer->title),
      '#weight' => -5,
    );
  }

  // Build fields content.
  field_attach_prepare_view('answer', array($answer->answer_id => $answer), $view_mode);
  entity_prepare_view('answer', array($answer->answer_id => $answer));
  $answer->content += field_attach_view('answer', $answer, $view_mode);
  
  return $answer->content;
} // function answer_page_view()


