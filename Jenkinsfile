#!/usr/bin/env groovy


def cppunit_test_exec = '${WORKSPACE}@tmp/cpp_unit_tests'
def python_version = params.getOrDefault("python_version", "3.9")
def python_image = "python:${python_version}"
def python_container_cmd = ''' \
  -u root \
  -v ${WORKSPACE}@tmp:/workspace \
  -v ${WORKSPACE}:/sources \
  -e COVERALLS_REPO_TOKEN \
  -e BUILD_NUMBER \
  -e GIT_BRANCH \
  -e TRAVIS_BRANCH="${GIT_BRANCH}" \
  -e TRAVIS_JOB_ID="${BUILD_NUMBER}"
  -e CI_PULL_REQUEST \
'''


def telegram_notification(message) {
  withCredentials([
    string(credentialsId: 'telegramBotToken', variable: 'BOT_TOKEN'),
    string(credentialsId: 'telegramChatId', variable: 'CHAT_ID')
  ]) {
    def telegram_url = 'https://api.telegram.org/bot${BOT_TOKEN}/sendMessage'
    message          = "-d text='${message}'"

    sh 'curl -X POST -d chat_id=${CHAT_ID} -d parse_mode=HTML ' + message + ' ' + telegram_url
  }
}


pipeline {

  agent any

  parameters {
    string(
      name: 'python_version',
      defaultValue: '3.9'
    )
  }

  stages {
    stage('Prepare'){
      steps {
        script {
            docker.image(python_image).inside(python_container_cmd){
                sh "python -m venv /workspace/venv"
            }
        }
      }
    }

    stage('Install'){
      steps {
        script {
            docker.image(python_image).inside(python_container_cmd){
                sh "cd /sources && /workspace/venv/bin/pip install -r requirements.txt"
                sh "cd /sources && /workspace/venv/bin/pip install -v '.[all,test]'"  // test should be
                // set explicitly for the Python:3.6
            }
        }
      }
    }

    stage('Python Test'){
      steps {
        script {
            docker.image(python_image).inside(python_container_cmd){
                sh "cd /sources/tests && /workspace/venv/bin/py.test -c pytest.ini"
            }
        }
      }
    }

    stage('CPP Tests'){
      steps {
        script {
          sh "${WORKSPACE}/tests/cpptests.sh ${cppunit_test_exec}"

          withCredentials([
            string(credentialsId: 'coveralls-wasp-c-extensions-token', variable: 'COVERALLS_REPO_TOKEN'),
          ]){
            docker.image(python_image).inside(python_container_cmd){
              sh "echo 'service_name: jenkins' > coveralls.yml"
              sh "cd /sources && /workspace/venv/bin/cpp-coveralls --coveralls-yaml coveralls.yml --include wasp_c_extensions"
            }
          }
        }
      }
    }

  }  // stages

  post {

    always {
      script{
        sh "rm -f ${cppunit_test_exec}"
        docker.image(python_image).inside(python_container_cmd){
          sh "rm -rf /workspace/venv/"
        }
      }
    }

    fixed { 
      script {
        message = "☘ The job <b>'${env.JOB_NAME}'</b> fixed. Details: ${env.BUILD_URL}"
        telegram_notification(message)
      }
    }
        
    aborted {
      script {        
        message = "🧯 The job <b>'${env.JOB_NAME}'</b> aborted. Details: ${env.BUILD_URL}"
        telegram_notification(message)
      }
    }
    
    failure {
      script {
        message = "🧯 The job <b>'${env.JOB_NAME}'</b> failed. Details: ${env.BUILD_URL}"
        telegram_notification(message)
      }
    }
    
    success {
      script {
        message = "☘ The job <b>'${env.JOB_NAME}'</b> completed successfully. Details: ${env.BUILD_URL}"
        telegram_notification(message) 
      }
    }    
        
  }
    
}
