<?php

/**
 * @file
 * Contains \Drupal\drupal8_greenhouse\JobPostAPIController.
 */

namespace Drupal\drupal8_greenhouse\Controller;

use Drupal\Core\Controller\ControllerBase;

use Symfony\Component\HttpFoundation\Request;
use Symfony\Component\HttpFoundation\JsonResponse;

/**
 * Controller routines for job_post_api routes.
 */

class JobPostAPIController extends ControllerBase {

  /**
   * {@inheritdoc}
   */

  public function get_job_count( Request $request ) {

    $query = \Drupal::entityQuery('node')
        ->condition('status', 1)
        ->condition('type', 'greenhouse_job_post')
        ->count();

    $result  = $query->execute() ;

    $response['data'] = $result;
    $response['method'] = 'GET';

    return new JsonResponse( $response );
  }
}