<?php

/**
 * Menu callback; List all exam types available.
 */
function exam_overview_types() {
  foreach (exam_types() as $type => $info) {
    $type_url_str = str_replace('_', '-', $type);
    $label = t('View @type', array('@type' => $info->name));
    $items[] = l($label, 'admin/structure/exams/manage/' . $type_url_str);
  }

  return theme('item_list', array('items' => $items));
}

/**
 * Menu callback; Exam information page.
 *
 * @param object $exam_type
 */
function exam_information($exam_type) {
  return $exam_type->name . ': ' . $exam_type->description;
}
