<?php

/**
 * @file
 * Admin page callbacks for the block module.
 */

/**
 * Page callback; Display a list of all custom blocks.
 */
function block_admin_list() {
  $custom_blocks_info = block_block_info();

  $header = array(
    t('Block'),
    t('Operations'),
  );

  $rows = array();
  foreach ($custom_blocks_info as $delta => $block_info) {
    $row = array();
    $row[] = check_plain($block_info['info']);
    $links = array();
    $links['configure'] = array(
      'title' => t('Configure'),
      'href' => 'admin/structure/block/manage/' . $delta . '/configure',
    );
    $links['delete'] = array(
      'title' => t('Delete'),
      'href' => 'admin/structure/block/manage/' . $delta . '/delete',
    );
    if (module_exists('translation')) {
      $links['translate'] = array(
        'title' => t('Translate'),
        'href' => 'admin/structure/block/manage/' . $delta . '/translate',
      );
    }
    $operations = array(
      '#type' => 'operations',
      '#links' => $links,
    );
    $row[] = array('data' => $operations);
    $rows[] = $row;
  }

  return array(
    '#theme' => 'table__block_admin_list',
    '#rows' => $rows,
    '#header' => $header,
    '#empty' => t('No custom blocks have been created yet.'),
  );
}

/**
 * Form constructor for the block configuration form.
 *
 * Also used by block_add_block_form() for adding a new custom block.
 *
 * @param $module
 *   Name of the module that implements the block to be configured.
 * @param $delta
 *   Unique ID of the block within the context of $module.
 *
 * @see block_menu()
 * @see block_admin_configure_validate()
 * @see block_admin_configure_submit()
 * @ingroup forms
 */
function block_admin_configure($form, &$form_state, $delta = NULL) {
  if ($delta) {
    $custom_block = block_custom_block_load($delta);
  }
  else {
    $custom_block = array(
      'info' => '',
      'title' => '',
      'body' => array('value' => '', 'format' => NULL),
    );
    if (module_exists('language')) {
      $custom_block['language_select'] => array(),
    }
  }
  if ($custom_block['info']) {
    backdrop_set_title(t("'%name' block", array('%name' => $custom_block['info'])), PASS_THROUGH);
  }
  else {
    backdrop_set_title(t('Add custom block'));
  }

  // Call our own block configuration form (also used by Layout module).
  $form += block_custom_block_form($custom_block, TRUE);

  // Prepare cancel link.
  if (isset($_GET['destination'])) {
    $path = $_GET['destination'];
  }
  elseif (isset($_SERVER['HTTP_REFERER'])) {
    $path = $_SERVER['HTTP_REFERER'];
  }
  elseif (user_access('administer blocks')) {
    $path = 'admin/structure/block';
  }
  else {
    $path = '<front>';
  }
  $options = backdrop_parse_url($path);
  $options['attributes']['class'][] = 'form-cancel';

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save block'),
  );
  $form['actions']['cancel'] = array(
    '#type' => 'link',
    '#title' => t('Cancel'),
    '#href' => $options['path'],
    '#options' => $options,
    '#weight' => 1,
  );

  return $form;
}

/**
 * Form validation handler for block_admin_configure().
 *
 * @see block_admin_configure()
 * @see block_admin_configure_submit()
 */
function block_admin_configure_validate($form, &$form_state) {
  $blocks = block_block_info();
  foreach ($blocks as $delta => $block) {
    if ($delta != $form_state['values']['delta'] && $block['info'] == $form_state['values']['info']) {
      form_set_error('info', t('Ensure that each block description is unique.'));
      return;
    }
  }
}

/**
 * Form submission handler for block_admin_configure().
 *
 * @see block_admin_configure()
 * @see block_admin_configure_validate()
 */
function block_admin_configure_submit($form, &$form_state) {
  block_custom_block_save($form_state['values'], $form_state['values']['delta']);
  backdrop_set_message(t('The block configuration has been saved.'));
  $form_state['redirect'] = 'admin/structure/block';
}

/**
 * Form constructor for the add block form.
 *
 * @see block_menu()
 * @see block_add_block_form_validate()
 * @see block_add_block_form_submit()
 * @ingroup forms
 */
function block_add_block_form($form, &$form_state) {
  return block_admin_configure($form, $form_state);
}

/**
 * Form validation handler for block_add_block_form().
 *
 * @see block_add_block_form()
 * @see block_add_block_form_submit()
 */
function block_add_block_form_validate($form, &$form_state) {
  return block_admin_configure_validate($form, $form_state);
}

/**
 * Form submission handler for block_add_block_form().
 *
 * Saves the new custom block.
 *
 * @see block_add_block_form()
 * @see block_add_block_form_validate()
 */
function block_add_block_form_submit($form, &$form_state) {
  block_custom_block_save($form_state['values'], $form_state['values']['delta']);
  backdrop_set_message(t('The block has been created.'));
  $form_state['redirect'] = 'admin/structure/block';
}

/**
 * Form constructor for the custom block deletion form.
 *
 * @param $module
 *   The name of the module that implements the block to be deleted. This should
 *   always equal 'block' since it only allows custom blocks to be deleted.
 * @param $delta
 *   The unique ID of the block within the context of $module.
 *
 * @see block_menu()
 * @see block_custom_block_delete_submit()
 */
function block_custom_block_delete($form, &$form_state, $delta) {
  $custom_block = block_custom_block_load($delta);
  $form['info'] = array('#type' => 'hidden', '#value' => $custom_block['info']);
  $form['delta'] = array('#type' => 'hidden', '#value' => $delta);

  return confirm_form($form, t('Are you sure you want to delete the block %name?', array('%name' => $custom_block['info'])), 'admin/structure/block', '', t('Delete'), t('Cancel'));
}

/**
 * Form submission handler for block_custom_block_delete().
 *
 * @see block_custom_block_delete()
 */
function block_custom_block_delete_submit($form, &$form_state) {
  config('block.custom.' . $form_state['values']['delta'])->delete();
  backdrop_set_message(t('The block %name has been removed.', array('%name' => $form_state['values']['info'])));
  $form_state['redirect'] = 'admin/structure/block';
}


function block_custom_block_translate_list($form, &$form_state, $delta) {
  $custom_block = block_custom_block_load($delta);

  $language_list = language_list();

  $header = array(
    t('Language'),
    t('Translated'),
    t('Operations'),
  );

  foreach ($language_list as $langcode => $item) {
    $row = array();
    $row[] = t($item->name);
    $row[] = isset($custom_block['translations'][$langcode]) ? t('Translated') : t('Not Translated');
    $links = array();
    $links['edit'] = array(
      'title' => t('Edit'),
      'href' => 'admin/structure/block/manage/' . $delta . '/translate/' . $langcode,
    );
    $operations = array(
      '#type' => 'operations',
      '#links' => $links,
    );
    $row[] = array('data' => $operations);
    $rows[] = $row;
  }

  return array(
    '#theme' => 'table__block_admin_list',
    '#rows' => $rows,
    '#header' => $header,
    '#empty' => t('No languages found to translate to.'),
  );
}



function block_custom_block_translate($form, &$form_state, $delta, $langcode) {
  $form['langcode'] = array('#type' => 'hidden', '#value' => $langcode);

  return block_admin_configure($form, $form_state, $delta);
}

/**
 * Form validation handler for block_custom_block_translate().
 *
 * @see block_custom_block_translate()
 * @see block_custom_block_translate_submit()
 */
function block_custom_block_translate_validate($form, &$form_state) {
  return block_admin_configure_validate($form, $form_state);
}

/**
 * Form submission handler for block_custom_block_translate().
 *
 * Saves the new custom block.
 *
 * @see block_custom_block_translate()
 * @see block_custom_block_translate_validate()
 */
function block_custom_block_translate_submit($form, &$form_state) {
  $langcode = NULL;
  if (isset($form_state['values']['langcode'])) {
    $langcode = $form_state['values']['langcode'];
  }
  block_custom_block_save($form_state['values'], $form_state['values']['delta'], $langcode);
  backdrop_set_message(t('The block has been created.'));
  $form_state['redirect'] = 'admin/structure/block';
}