<?php

/**
 * Controller for loading, creating, and saving exams.
 *
 * The default loader, which we extend, handles load() already. We only
 * need to add saving and creating.
 */
class ExamController extends DrupalDefaultEntityController {

  public function save($exam) {
    $transaction = db_transaction();

    try {
      global $user;

      // Determine if we will be inserting a new exam.
      $exam->is_new = empty($exam->exam_id);

      // Set the timestamp fields.
      if (empty($exam->created)) {
        $exam->created = REQUEST_TIME;
      }
      $exam->changed = REQUEST_TIME;

      $exam->revision_timestamp = REQUEST_TIME;
      $update_exam = TRUE;

      // Give modules the opportunity to prepare field data for saving.
      field_attach_presave('exam', $exam);

      // When saving a new exam revision, unset any existing $exam->vid
      // to ensure a new revision will actually be created and store the old
      // revision ID in a separate property for exam hook implementations.
      if (!$exam->is_new && !empty($exam->revision) && $exam->vid) {
        $exam->old_vid = $exam->vid;
        unset($exam->vid);
      }

      // If this is a new exam...
      if ($exam->is_new) {
        // Save the new exam.
        drupal_write_record('exam', $exam);

        // Save the initial revision.
        $this->saveRevision($exam, $user->uid);

        $op = 'insert';
      }
      else {
        // Save the updated exam.
        drupal_write_record('exam', $exam, 'exam_id');

        // If a new exam revision was requested, save a new record for that;
        // otherwise, update the exam revision record that matches the value
        // of $exam->vid.
        if (!empty($exam->revision)) {
          $this->saveRevision($exam, $user->uid);
        }
        else {
          $this->saveRevision($exam, $user->uid, TRUE);
          $update_exam = FALSE;
        }

        $op = 'update';
      }

      // If the revision ID is new or updated, save it to the exam.
      if ($update_exam) {
        db_update('exam')
          ->fields(array('vid' => $exam->vid))
          ->condition('exam_id', $exam->exam_id)
          ->execute();
      }

      // Save fields.
      $function = 'field_attach_' . $op;
      $function('exam', $exam);

      module_invoke_all('entity_' . $op, $exam, 'exam');

      // Clear internal properties.
      unset($exam->is_new);

      // Ignore slave server temporarily to give time for the saved order to be
      // propagated to the slave.
      db_ignore_slave();
      
      // SKERE - this is to ensure the menu is rebuilt to include the exam     
      menu_rebuild();

      return $exam;
    }
    catch (Exception $e) {
      $transaction->rollback();
      watchdog_exception('exam', $e, NULL, WATCHDOG_ERROR);
      return FALSE;
    }
  }

  /**
   * Saves an exam revision with the uid of the current user.
   *
   * @param $exam
   *   The fully loaded exam object.
   * @param $uid
   *   The user's uid for the current revision.
   * @param $update
   *   TRUE or FALSE indicating whether or not the existing revision should be
   *     updated instead of a new one created.
   */
  function saveRevision($exam, $uid, $update = FALSE) {
    // Hold on to the exam's original creator_uid but swap in the revision's
    // creator_uid for the momentary write.
    $temp_uid = $exam->uid;
    $exam->uid = $uid;

    // Update the existing revision if specified.
    if ($update) {
      drupal_write_record('exam_revision', $exam, 'vid');
    }
    else {
      // Otherwise insert a new revision. This will automatically update $exam
      // to include the vid.
      drupal_write_record('exam_revision', $exam);
    }

    // Reset the order's creator_uid to the original value.
    $exam->uid = $temp_uid;
 }

  /**
   * Deletes multiple exams by ID.
   *
   * @param $exam_ids
   *   An array of exam IDs to delete.
   * @return
   *   TRUE on success, FALSE otherwise.
   */
  public function delete($exam_ids) {
    if (!empty($exam_ids)) {
      $exams = $this->load($exam_ids, array());

      $transaction = db_transaction();

      try {
        db_delete('exam')
          ->condition('exam_id', $exam_ids, 'IN')
          ->execute();

        db_delete('exam_revision')
          ->condition('exam_id', $exam_ids, 'IN')
          ->execute();

        foreach ($exams as $exam_id => $exam) {
          field_attach_delete('exam', $exam);
        }

        // Ignore slave server temporarily to give time for the
        // saved exam to be propagated to the slave.
        db_ignore_slave();
      }
      catch (Exception $e) {
        $transaction->rollback();
        watchdog_exception('exam', $e, NULL, WATCHDOG_ERROR);
        return FALSE;
      }

      module_invoke_all('entity_delete', $exam, 'exam');

      // Clear the page and block and exam caches.
      cache_clear_all();
      $this->resetCache();
    }

    return TRUE;
  }

  /**
   * Create a default exam.
   *
   * @param $type
   *   The machine-readable type of the exam.
   *
   * @return
   *   An exam object with all default fields initialized.
   */
  public function create($type = '') {
    return (object) array(
      'exam_id' => '',
      'type' => $type,
      'exam_title' => '',
      'exam_description' => '',
      'status' => '',
    );
  }
}

/**
 * Controller for loading, creating, and saving topics.
 *
 * The default loader, which we extend, handles load() already. We only
 * need to add saving and creating.
 */
class TopicController extends \DrupalDefaultEntityController {

  public function save($topic) {
    $transaction = db_transaction();

    try {
      global $user;

      // Determine if we will be inserting a new topic.
      $topic->is_new = empty($topic->topic_id);

      // Set the timestamp fields.
      if (empty($topic->created)) {
        $topic->created = REQUEST_TIME;
      }
      $topic->changed = REQUEST_TIME;

      $topic->revision_timestamp = REQUEST_TIME;
      $update_topic = TRUE;

      // Give modules the opportunity to prepare field data for saving.
      field_attach_presave('topic', $topic);

      // When saving a new topic revision, unset any existing $topic->vid
      // to ensure a new revision will actually be created and store the old
      // revision ID in a separate property for topic hook implementations.
      if (!$topic->is_new && !empty($topic->revision) && $topic->vid) {
        $topic->old_vid = $topic->vid;
        unset($topic->vid);
      }

      // If this is a new topic...
      if ($topic->is_new) {
        // Save the new topic.
        drupal_write_record('topic', $topic);

        // Save the initial revision.
        $this->saveRevision($topic, $user->uid);

        $op = 'insert';
      }
      else {
        // Save the updated topic.
        drupal_write_record('topic', $topic, 'topic_id');

        // If a new topic revision was requested, save a new record for that;
        // otherwise, update the topic revision record that matches the value
        // of $topic->vid.
        if (!empty($topic->revision)) {
          $this->saveRevision($topic, $user->uid);
        }
        else {
          $this->saveRevision($topic, $user->uid, TRUE);
          $update_topic = FALSE;
        }

        $op = 'update';
      }

      // If the revision ID is new or updated, save it to the topic.
      if ($update_topic) {
        db_update('topic')
          ->fields(array('vid' => $topic->vid))
          ->condition('topic_id', $topic->topic_id)
          ->execute();
      }

      // Save fields.
      $function = 'field_attach_' . $op;
      $function('topic', $topic);

      module_invoke_all('entity_' . $op, $topic, 'topic');

      // Clear internal properties.
      unset($topic->is_new);

      // Ignore slave server temporarily to give time for the saved order to be
      // propagated to the slave.
      db_ignore_slave();

      return $topic;
    }
    catch (Exception $e) {
      $transaction->rollback();
      watchdog_exception('topic', $e, NULL, WATCHDOG_ERROR);
      return FALSE;
    }
  }

  /**
   * Saves a topic revision with the uid of the current user.
   *
   * @param $topic
   *   The fully loaded topic object.
   * @param $uid
   *   The user's uid for the current revision.
   * @param $update
   *   TRUE or FALSE indicating whether or not the existing revision should be
   *     updated instead of a new one created.
   */
  function saveRevision($topic, $uid, $update = FALSE) {
    // Hold on to the topic's original creator_uid but swap in the revision's
    // creator_uid for the momentary write.
    $temp_uid = $topic->uid;
    $topic->uid = $uid;

    // Update the existing revision if specified.
    if ($update) {
      drupal_write_record('topic_revision', $topic, 'vid');
    }
    else {
      // Otherwise insert a new revision. This will automatically update $topic
      // to include the vid.
      drupal_write_record('topic_revision', $topic);
    }

    // Reset the order's creator_uid to the original value.
    $topic->uid = $temp_uid;
 }

  /**
   * Deletes multiple topics by ID.
   *
   * @param $topic_ids
   *   An array of topic IDs to delete.
   * @return
   *   TRUE on success, FALSE otherwise.
   */
  public function delete($topic_ids) {
    if (!empty($topic_ids)) {
      $topics = $this->load($topic_ids, array());

      $transaction = db_transaction();

      try {
        db_delete('topic')
          ->condition('topic_id', $topic_ids, 'IN')
          ->execute();

        db_delete('topic_revision')
          ->condition('topic_id', $topic_ids, 'IN')
          ->execute();

        foreach ($topics as $topic_id => $topic) {
          field_attach_delete('topic', $topic);
        }

        // Ignore slave server temporarily to give time for the
        // saved topic to be propagated to the slave.
        db_ignore_slave();
      }
      catch (Exception $e) {
        $transaction->rollback();
        watchdog_exception('topic', $e, NULL, WATCHDOG_ERROR);
        return FALSE;
      }

      module_invoke_all('entity_delete', $topic, 'topic');

      // Clear the page and block and topic caches.
      cache_clear_all();
      $this->resetCache();
    }

    return TRUE;
  }

  /**
   * Create a default topic.
   *
   * @param $type
   *   The machine-readable type of the topic.
   *
   * @return
   *   A topic object with all default fields initialized.
   */
  public function create() {
    return (object) array(
      'topic_id' => '',
      'exam_id' => '',
      'topic_title' => '',
      'status' => '',
    );
  }
}

/**
 * Controller for loading, creating, and saving questions.
 *
 * The default loader, which we extend, handles load() already. We only
 * need to add saving and creating.
 */
class QuestionController extends DrupalDefaultEntityController {

  public function save($question) {
    $transaction = db_transaction();

    try {
      global $user;

      // Determine if we will be inserting a new question.
      $question->is_new = empty($question->question_id);

      // Set the timestamp fields.
      if (empty($question->created)) {
        $question->created = REQUEST_TIME;
      }
      $question->changed = REQUEST_TIME;

      $question->revision_timestamp = REQUEST_TIME;
      $update_question = TRUE;

      // Give modules the opportunity to prepare field data for saving.
      field_attach_presave('question', $question);

      // When saving a new question revision, unset any existing $question->vid
      // to ensure a new revision will actually be created and store the old
      // revision ID in a separate property for question hook implementations.
      if (!$question->is_new && !empty($question->revision) && $question->vid) {
        $question->old_vid = $question->vid;
        unset($question->vid);
      }

      // If this is a new question...
      if ($question->is_new) {
        // Save the new question.
        drupal_write_record('question', $question);

        // Save the initial revision.
        $this->saveRevision($question, $user->uid);

        $op = 'insert';
      }
      else {
        // Save the updated question.
        drupal_write_record('question', $question, 'question_id');

        // If a new question revision was requested, save a new record for that;
        // otherwise, update the question revision record that matches the value
        // of $question->vid.
        if (!empty($question->revision)) {
          $this->saveRevision($question, $user->uid);
        }
        else {
          $this->saveRevision($question, $user->uid, TRUE);
          $update_question = FALSE;
        }

        $op = 'update';
      }

      // If the revision ID is new or updated, save it to the question.
      if ($update_question) {
        db_update('question')
          ->fields(array('vid' => $question->vid))
          ->condition('question_id', $question->question_id)
          ->execute();
      }

      // Save fields.
      $function = 'field_attach_' . $op;
      $function('question', $question);

      module_invoke_all('entity_' . $op, $question, 'question');

      // Clear internal properties.
      unset($question->is_new);

      // Ignore slave server temporarily to give time for the saved order to be
      // propagated to the slave.
      db_ignore_slave();

      return $question;
    }
    catch (Exception $e) {
      $transaction->rollback();
      watchdog_exception('question', $e, NULL, WATCHDOG_ERROR);
      return FALSE;
    }
  }

  /**
   * Saves a question revision with the uid of the current user.
   *
   * @param $question
   *   The fully loaded question object.
   * @param $uid
   *   The user's uid for the current revision.
   * @param $update
   *   TRUE or FALSE indicating whether or not the existing revision should be
   *     updated instead of a new one created.
   */
  function saveRevision($question, $uid, $update = FALSE) {
    // Hold on to the question's original creator_uid but swap in the revision's
    // creator_uid for the momentary write.
    $temp_uid = $question->uid;
    $question->uid = $uid;

    // Update the existing revision if specified.
    if ($update) {
      drupal_write_record('question_revision', $question, 'vid');
    }
    else {
      // Otherwise insert a new revision. This will automatically update $question
      // to include the vid.
      drupal_write_record('question_revision', $question);
    }

    // Reset the order's creator_uid to the original value.
    $question->uid = $temp_uid;
 }

  /**
   * Deletes multiple questions by ID.
   *
   * @param $question_ids
   *   An array of question IDs to delete.
   * @return
   *   TRUE on success, FALSE otherwise.
   */
  public function delete($question_ids) {
    if (!empty($question_ids)) {
      $questions = $this->load($question_ids, array());

      $transaction = db_transaction();

      try {
        db_delete('question')
          ->condition('question_id', $question_ids, 'IN')
          ->execute();

        db_delete('question_revision')
          ->condition('question_id', $question_ids, 'IN')
          ->execute();

        foreach ($questions as $question_id => $question) {
          field_attach_delete('question', $question);
        }

        // Ignore slave server temporarily to give time for the
        // saved question to be propagated to the slave.
        db_ignore_slave();
      }
      catch (Exception $e) {
        $transaction->rollback();
        watchdog_exception('question', $e, NULL, WATCHDOG_ERROR);
        return FALSE;
      }

      module_invoke_all('entity_delete', $question, 'question');

      // Clear the page and block and question caches.
      cache_clear_all();
      $this->resetCache();
    }

    return TRUE;
  }

  /**
   * Create a default topic.
   *
   * @param $type
   *   The machine-readable type of the question.
   *
   * @return
   *   A question object with all default fields initialized.
   */
  public function create() {
    return (object) array(
      'question_id' => '',
      'exam_id' => '',
      'topic_id' => '',
      'question_text' => '',
      'status' => '',
    );
  }
}

/**
 * Controller for loading, creating, and saving answers.
 *
 * The default loader, which we extend, handles load() already. We only
 * need to add saving and creating.
 */
class AnswerController extends DrupalDefaultEntityController {

  public function save($answer) {
    $transaction = db_transaction();

    try {
      global $user;

      // Determine if we will be inserting a new answer.
      $answer->is_new = empty($answer->answer_id);

      // Set the timestamp fields.
      if (empty($answer->created)) {
        $answer->created = REQUEST_TIME;
      }
      $answer->changed = REQUEST_TIME;

      $answer->revision_timestamp = REQUEST_TIME;
      $update_answer = TRUE;

      // Give modules the opportunity to prepare field data for saving.
      field_attach_presave('answer', $answer);

      // When saving a new answer revision, unset any existing $answer->vid
      // to ensure a new revision will actually be created and store the old
      // revision ID in a separate property for answer hook implementations.
      if (!$answer->is_new && !empty($answer->revision) && $answer->vid) {
        $answer->old_vid = $answer->vid;
        unset($answer->vid);
      }

      // If this is a new answer...
      if ($answer->is_new) {
        // Save the new answer.
        drupal_write_record('answer', $answer);

        // Save the initial revision.
        $this->saveRevision($answer, $user->uid);

        $op = 'insert';
      }
      else {
        // Save the updated answer.
        drupal_write_record('answer', $answer, 'answer_id');

        // If a new answer revision was requested, save a new record for that;
        // otherwise, update the answer revision record that matches the value
        // of $answer->vid.
        if (!empty($answer->revision)) {
          $this->saveRevision($answer, $user->uid);
        }
        else {
          $this->saveRevision($answer, $user->uid, TRUE);
          $update_answer = FALSE;
        }

        $op = 'update';
      }

      // If the revision ID is new or updated, save it to the answer.
      if ($update_answer) {
        db_update('answer')
          ->fields(array('vid' => $answer->vid))
          ->condition('answer_id', $answer->answer_id)
          ->execute();
      }

      // Save fields.
      $function = 'field_attach_' . $op;
      $function('answer', $answer);

      module_invoke_all('entity_' . $op, $answer, 'answer');

      // Clear internal properties.
      unset($answer->is_new);

      // Ignore slave server temporarily to give time for the saved order to be
      // propagated to the slave.
      db_ignore_slave();

      return $answer;
    }
    catch (Exception $e) {
      $transaction->rollback();
      watchdog_exception('answer', $e, NULL, WATCHDOG_ERROR);
      return FALSE;
    }
  }

  /**
   * Saves an answer revision with the uid of the current user.
   *
   * @param $question
   *   The fully loaded answer object.
   * @param $uid
   *   The user's uid for the current revision.
   * @param $update
   *   TRUE or FALSE indicating whether or not the existing revision should be
   *     updated instead of a new one created.
   */
  function saveRevision($answer, $uid, $update = FALSE) {
    // Hold on to the answer's original creator_uid but swap in the revision's
    // creator_uid for the momentary write.
    $temp_uid = $answer->uid;
    $answer->uid = $uid;

    // Update the existing revision if specified.
    if ($update) {
      drupal_write_record('answer_revision', $answer, 'vid');
    }
    else {
      // Otherwise insert a new revision. This will automatically update $answer
      // to include the vid.
      drupal_write_record('answer_revision', $answer);
    }

    // Reset the order's creator_uid to the original value.
    $answer->uid = $temp_uid;
 }

  /**
   * Deletes multiple answers by ID.
   *
   * @param $answer_ids
   *   An array of answer IDs to delete.
   * @return
   *   TRUE on success, FALSE otherwise.
   */
  public function delete($answer_ids) {
    if (!empty($answer_ids)) {
      $answers = $this->load($answer_ids, array());

      $transaction = db_transaction();

      try {
        db_delete('answer')
          ->condition('answer_id', $answer_ids, 'IN')
          ->execute();

        db_delete('answer_revision')
          ->condition('answer_id', $answer_ids, 'IN')
          ->execute();

        foreach ($answers as $answer_id => $answer) {
          field_attach_delete('answer', $answer);
        }

        // Ignore slave server temporarily to give time for the
        // saved answer to be propagated to the slave.
        db_ignore_slave();
      }
      catch (Exception $e) {
        $transaction->rollback();
        watchdog_exception('answer', $e, NULL, WATCHDOG_ERROR);
        return FALSE;
      }

      module_invoke_all('entity_delete', $answer, 'answer');

      // Clear the page and block and answer caches.
      cache_clear_all();
      $this->resetCache();
    }

    return TRUE;
  }

  /**
   * Create a default topic.
   *
   * @param $type
   *   The machine-readable type of the answer.
   *
   * @return
   *   An answer object with all default fields initialized.
   */
  public function create() {
    return (object) array(
      'answer_id' => '',
      'question_id' => '',
      'answer_text' => '',
      'is_correct' => '',
      'rationale' => '',
      'status' => '',
    );
  }
}
