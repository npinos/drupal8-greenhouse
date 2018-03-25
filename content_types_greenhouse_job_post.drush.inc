<?php

include_once 'GreenhouseJobBoardAPI/vendor/autoload.php';
include_once 'GreenhouseJobBoardAPI/src/GreenhouseJobBoardAPI.php';
use GreenhouseJobBoardAPI\GreenhouseJobBoardAPI;
use Drupal\node\Entity\Node;

/**
* Implements hook_drush_command().
*/
function drupal8_greenhouse_drush_command() {
  $items = array();
  // In this example the module requires the existence of a module defining the job post content type.
  $items['greenhouse-sync'] = [
    'description' => 'Sync with greenhouse',
    'drupal dependencies' => ['content_types_greenhouse_job_post'],
    'aliases' => ['gs'],
  ];
  return $items;
}
/**
* The call back function name in the  following format
*   drush_{module_name}_{item_id_for_command}()
*/
function drupal8_greenhouse_greenhouse_sync() {
  $api_url = "https://api.greenhouse.io/v1/boards/ACCOUNT_ID/";
  $greenhouse = new GreenhouseJobBoardAPI($api_url);

  $jobs = $greenhouse->getJobs(true);
  $gh_job_index = array();
  $jobs_created = 0;
  $jobs_updated = 0;

  if($jobs->jobs != NULL) {
    foreach ($jobs->jobs as $gh_job_post) {

      $gh_job_index[] = $gh_job_post->id;

      $local_job_post = job_post_load($gh_job_post->id);

      if($local_job_post != false) {
        if(job_post_needs_update($local_job_post, $gh_job_post)) {
          $job_obj = $greenhouse->getJob($gh_job_post->id);
          update_job_post($local_job_post, $job_obj);
          $jobs_updated++;
        }
      }
      else {
        $job_obj = $greenhouse->getJob($gh_job_post->id);
        if(is_object($job_obj)) {
          create_job_post($job_obj);
          $jobs_created++;
        }
      }
    }
    print($jobs_created . ' Job posts created' . PHP_EOL);
    print($jobs_updated . ' Job posts updated' . PHP_EOL);
    purge_jobs_database($gh_job_index);
  }
}

/**
 * Loads a job post from the Drupal database.
 */
function job_post_load($gh_job_post_id) {
    $query = \Drupal::entityQuery('node')
        ->condition('status', 1)
        ->condition('type', 'greenhouse_job_post')
        ->condition('field_green_house_id', $gh_job_post_id);

    $result = $query->execute();
    $nodes = \Drupal\node\Entity\Node::loadMultiple($result);
    $node = array_shift($nodes);
    if($node) {
      return $node;
    }
    else {
      return false;
    }
}

/**
 * Compares local and remote copy.
 */
function job_post_needs_update($local_job_post, $gh_job_post) {
    if ($local_job_post->field_greenhouse_last_updated && ($gh_job_post->updated_at != $local_job_post->field_greenhouse_last_updated->getValue()[0]["value"])) {
      return true;
    }
    return false;
}

/**
 * @todo Create a real update function.
 */
function update_job_post($node, $gh_job_post) {
  $node->delete();
  create_job_post($gh_job_post);
}

/**
 * Purge database from posts that have been deleted from GH
 */
function purge_jobs_database($gh_job_index) {
  $deleted_job_posts = 0;
  $query = \Drupal::entityQuery('node')
    ->condition('status', 1)
    ->condition('type', 'greenhouse_job_post');

  $result = $query->execute();
  $local_job_posts = \Drupal\node\Entity\Node::loadMultiple($result);

  if ($local_job_posts) {
    foreach ($local_job_posts as $job) {
      if(!in_array($job->field_green_house_id->getValue()[0]["value"], $gh_job_index)) {
        $job->delete();
        $deleted_job_posts++;
      }
    }
    print($deleted_job_posts . " Job posts deleted" . PHP_EOL);
  }
}

/**
 * Creates a job post().
 */
function create_job_post($gh_job_post) {
    $node = Node::create(array(
        'type' => 'greenhouse_job_post',
        'title' => $gh_job_post->title,
        'langcode' => 'en',
        'uid' => '1',
        'status' => 1,
        'body' => array(
          'value' => html_entity_decode($gh_job_post->content),
          'format' => 'full_html',
        ),
        'field_green_house_id' => array($gh_job_post->id),
        'field_job_post_location' => array($gh_job_post->location->name),
        'field_job_post_department' => array($gh_job_post->departments[0]->name),
        'path' =>  array('alias' => '/careers/apply/' . str_replace(' ', '-', strtolower($gh_job_post->title))),
        'field_greenhouse_last_updated' => array($gh_job_post->updated_at)
    ));
    $node->save();
}


